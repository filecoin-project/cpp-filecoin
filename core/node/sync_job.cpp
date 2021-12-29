/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "node/sync_job.hpp"

#include "common/error_text.hpp"
#include "common/logger.hpp"
#include "common/outcome_fmt.hpp"
#include "node/blocksync_common.hpp"
#include "node/chain_store_impl.hpp"
#include "node/events.hpp"
#include "node/fetch_msg.hpp"
#include "node/peer_height.hpp"
#include "vm/interpreter/interpreter.hpp"

namespace fc::sync {
  using primitives::tipset::chain::stepParent;

  constexpr auto kBranchCompactTreshold{200u};

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("sync_job");
      return logger.get();
    }
  }  // namespace

  outcome::result<TipsetCPtr> stepUp(const TsLoadPtr &ts_load,
                                     const TsBranchPtr &branch,
                                     const TipsetCPtr &ts) {
    if (branch->chain.rbegin()->second.key != ts->key) {
      OUTCOME_TRY(it, find(branch, ts->height() + 1, false));
      if (!it.first) {
        return nullptr;
      }
      OUTCOME_TRY(it2, stepParent(it));
      if (it2.second->second.key == ts->key) {
        return ts_load->lazyLoad(it.second->second);
      }
    }
    return ERROR_TEXT("stepUp: error");
  }

  SyncJob::SyncJob(std::shared_ptr<libp2p::Host> host,
                   std::shared_ptr<boost::asio::io_context> io,
                   std::shared_ptr<ChainStoreImpl> chain_store,
                   std::shared_ptr<Scheduler> scheduler,
                   std::shared_ptr<Interpreter> interpreter,
                   std::shared_ptr<InterpreterCache> interpreter_cache,
                   SharedMutexPtr ts_branches_mutex,
                   TsBranchesPtr ts_branches,
                   TsBranchPtr ts_main,
                   TsLoadPtr ts_load,
                   std::shared_ptr<PutBlockHeader> put_block_header,
                   IpldPtr ipld)
      : host_(std::move(host)),
        io_(std::move(io)),
        chain_store_(std::move(chain_store)),
        scheduler_(std::move(scheduler)),
        interpreter_(std::move(interpreter)),
        interpreter_cache_(std::move(interpreter_cache)),
        ts_branches_mutex_{std::move(ts_branches_mutex)},
        ts_branches_{std::move(ts_branches)},
        ts_main_(std::move(ts_main)),
        ts_load_(std::move(ts_load)),
        put_block_header_{std::move(put_block_header)},
        ipld_(std::move(ipld)) {
    attached_.insert(ts_main_);
  }

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  void SyncJob::start(std::shared_ptr<events::Events> events) {
    if (events_) {
      log()->error("already started");
      return;
    }

    events_ = std::move(events);
    assert(events_);

    message_event_ =
        events_->subscribeMessageFromPubSub([ipld{ipld_}](auto &e) {
          std::ignore = e.msg.signature.isBls() ? setCbor(ipld, e.msg.message)
                                                : setCbor(ipld, e.msg);
        });

    block_event_ = events_->subscribeBlockFromPubSub([=](auto &e) {
      auto ipld{ipld_};
      std::ignore = [&]() -> outcome::result<void> {
        primitives::tipset::put(ipld, put_block_header_, e.block.header);
        std::shared_lock ts_lock{*ts_branches_mutex_};
        primitives::block::MsgMeta meta;
        cbor_blake::cbLoadT(ipld, meta);
        for (auto &cid : e.block.bls_messages) {
          OUTCOME_TRY(ipld->get(cid));
          OUTCOME_TRY(meta.bls_messages.append(cid));
        }
        for (auto &cid : e.block.secp_messages) {
          OUTCOME_TRY(ipld->get(cid));
          OUTCOME_TRY(meta.secp_messages.append(cid));
        }
        OUTCOME_TRY(setCbor(ipld, meta));
        return outcome::success();
      }();
    });

    peers_ = std::make_shared<PeerHeight>(events_);

    fetch_msg_ = std::make_shared<FetchMsg>(host_, scheduler_, peers_, ipld_);
    fetch_msg_->on_fetch = [this](TipsetKey tsk) {
      std::unique_lock lock{*ts_branches_mutex_};
      if (interpret_ts_ && interpret_ts_->key == tsk) {
        interpretDequeue();
      }
    };

    possible_head_event_ =
        events_->subscribePossibleHead([this](const events::PossibleHead &e) {
          thread.io->post([=] { onPossibleHead(e); });
        });

    log()->debug("started");
  }

  ChainEpoch SyncJob::metricAttachedHeight() const {
    std::shared_lock lock{*ts_branches_mutex_};
    if (auto branch{attached_heaviest_.first}) {
      return branch->chain.rbegin()->first;
    }
    return 0;
  }

  void SyncJob::onPossibleHead(const events::PossibleHead &e) {
    if (auto ts{getLocal(e.head)}) {
      onTs(e.source, ts);
    } else if (e.source) {
      fetch(*e.source, e.head);
    }
  }

  TipsetCPtr SyncJob::getLocal(const TipsetKey &tsk) {
    if (auto _ts{ts_load_->load(tsk)}) {
      return _ts.value();
    }
    return nullptr;
  }

  void SyncJob::compactBranches() {
    std::pair<TsBranchPtr, size_t> longest{};
    for (const auto &head : *ts_branches_) {
      if (attached_.count(head) == 0) {
        size_t length{};
        for (auto branch{head}; branch; branch = branch->parent) {
          length += branch->chain.size();
        }
        if (!longest.first || longest.second < length) {
          longest = {head, length};
        }
      }
    }
    attached_ = {ts_main_};
    *ts_branches_ = {ts_main_};
    auto compact{[&](auto &head) -> TsBranchPtr {
      if (head == ts_main_) {
        return nullptr;
      }
      primitives::tipset::chain::TsChain chain;
      TsBranchPtr last;
      for (auto branch{head}; branch && branch != ts_main_;
           branch = branch->parent) {
        chain.insert(branch->chain.begin(), branch->chain.end());
        last = branch;
      }
      auto branch{TsBranch::make(std::move(chain), last->parent)};
      if (branch == ts_main_) {
        return nullptr;
      }
      branch->parent_key = last->parent_key;
      ts_branches_->insert(branch);
      return branch;
    }};
    if (auto &heaviest{attached_heaviest_.first}) {
      heaviest = compact(heaviest);
      if (heaviest) {
        attached_.insert(heaviest);
      }
    }
    if (longest.first) {
      compact(longest.first);
    }
  }

  void SyncJob::onTs(const boost::optional<PeerId> &peer, TipsetCPtr ts) {
    std::unique_lock lock{*ts_branches_mutex_};
    if (ts_branches_->size() > kBranchCompactTreshold) {
      log()->info("compacting branches");
      compactBranches();
    }
    auto batch{1000};
    while (true) {
      fetch_msg_->has(ts, false);
      std::vector<TsBranchPtr> children;
      auto branch{insert(*ts_branches_, ts, &children).first};
      if (attached_.count(branch) != 0) {
        auto last{attached_heaviest_.first};
        for (auto &child : children) {
          attach(child);
        }
        updateTarget(last);
      } else {
        while (branch->parent) {
          branch = branch->parent;
        }
        if (branch->parent_key) {
          if (auto _ts{getLocal(*branch->parent_key)}) {
            ts = _ts;
            --batch;
            if (batch <= 0) {
              thread.io->post([=] { onTs(peer, ts); });
              break;
            }
            continue;
          }
          if (peer) {
            fetch(*peer, *branch->parent_key);
          }
        }
      }
      break;
    }
    interpretDequeue();
  }

  void SyncJob::attach(TsBranchPtr branch) {
    std::vector<TsBranchPtr> queue{branch};
    while (!queue.empty()) {
      branch = queue.back();
      queue.pop_back();
      attached_.insert(branch);
      if (auto _ts{ts_load_->lazyLoad(branch->chain.rbegin()->second)}) {
        auto &ts{_ts.value()};
        const auto &w1{attached_heaviest_.second};
        const auto &w2{ts->getParentWeight()};
        if (w2 > w1
            || (attached_heaviest_.first && w2 == w1
                && ts->key.cids().size()
                       > attached_heaviest_.first->chain.rbegin()
                             ->second.key.cids()
                             .size())) {
          attached_heaviest_ = {branch, ts->getParentWeight()};
        }
      }
      for (auto &_child : branch->children) {
        if (auto child{_child.second.lock()}) {
          queue.push_back(child);
        }
      }
    }
  }

  void SyncJob::updateTarget(const TsBranchPtr &last) {
    auto branch{attached_heaviest_.first};
    if (branch == last) {
      return;
    }
    auto it{std::prev(branch->chain.end())};
    while (true) {
      if (auto _res{interpreter_cache_->tryGet(it->second.key)}) {
        if (*_res) {
          if (auto _ts{ts_load_->lazyLoad(it->second)}) {
            interpret_ts_ = _ts.value();
          }
        }
        break;
      }
      if (auto _it{stepParent({branch, it})}) {
        std::tie(branch, it) = _it.value();
      } else {
        break;
      }
      if (branch == ts_main_) {
        log()->info("main not interpreted {}", it->first);
        break;
      }
    }
  }

  void SyncJob::onInterpret(const TipsetCPtr &ts,
                            const InterpreterResult &result) {
    const auto &weight{result.weight};
    if (weight > chain_store_->getHeaviestWeight()) {
      auto branch{find(*ts_branches_, ts)};
      if (!branch.first) {
        log()->warn(
            "onInterpret no branch {} {}", ts->height(), ts->key.cidsStr());
        return;
      }
      if (const auto _update{update(ts_main_, branch)}) {
        const auto &[path, removed]{_update.value()};
        for (const auto &removed_branch : removed) {
          ts_branches_->erase(removed_branch);
          attached_.erase(removed_branch);
        }
        chain_store_->update(path, weight);
      } else {
        log()->error("update {:#}", _update.error());
      }
    }
  }

  bool SyncJob::checkParent(const TipsetCPtr &ts) {
    if (ts->height() != 0) {
      if (auto _res{interpreter_cache_->tryGet(ts->getParents())}) {
        if (*_res) {
          auto &res{_res->value()};
          const auto &a_receipts{res.message_receipts};
          const auto &e_receipts{ts->getParentMessageReceipts()};
          const auto &a_state{res.state_root};
          const auto &e_state{ts->getParentStateRoot()};
          if (a_state != e_state || a_receipts != e_receipts) {
            log()->warn("parent state mismatch {} {}, ({} {}) != ({} {})",
                        ts->height(),
                        ts->key.cidsStr(),
                        a_state,
                        a_receipts,
                        e_state,
                        e_receipts);
            return false;
          }
        } else {
          log()->warn(
              "parent interpret error {} {}", ts->height(), ts->key.cidsStr());
          return false;
        }
      } else {
        log()->warn(
            "parent not interpreted {} {}", ts->height(), ts->key.cidsStr());
        return false;
      }
      if (auto _has{ipld_->contains(ts->getParentStateRoot())};
          !_has || !_has.value()) {
        log()->warn("no parent state {} {}", ts->height(), ts->key.cidsStr());
        return false;
      }
    }
    return true;
  }

  void SyncJob::interpretDequeue() {
    if (interpreting_ || !interpret_ts_) {
      return;
    }
    auto ts{interpret_ts_};
    if (!fetch_msg_->has(ts, true)) {
      return;
    }
    auto branch{find(*ts_branches_, ts).first};
    if (!checkParent(ts)) {
      // TODO(turuslan): detach and ban branches
      interpret_ts_ = nullptr;
      return;
    }
    interpreting_ = true;
    interpret_thread.io->post([=] {
      auto result{interpreter_->interpret(branch, ts)};
      if (!result) {
        log()->warn("interpret error {:#} {} {}",
                    result.error(),
                    ts->height(),
                    ts->key.cidsStr());
      }
      thread.io->post([=] {
        std::unique_lock lock{*ts_branches_mutex_};
        interpreting_ = false;
        if (result) {
          onInterpret(ts, result.value());
        }
        if (interpret_ts_ == ts) {
          if (result) {
            if (auto _ts{stepUp(
                    ts_load_, attached_heaviest_.first, interpret_ts_)}) {
              interpret_ts_ = _ts.value();
            } else {
              interpret_ts_ = nullptr;
            }
          } else {
            // TODO(turuslan): detach and ban branches
            interpret_ts_ = nullptr;
          }
        }
        interpretDequeue();
      });
    });
  }

  void SyncJob::fetch(const PeerId &peer, const TipsetKey &tsk) {
    std::unique_lock lock{requests_mutex_};
    requests_.emplace(peer, tsk);
    lock.unlock();
    io_->post([=] { fetchDequeue(); });
  }

  void SyncJob::fetchDequeue() {
    std::lock_guard lock{requests_mutex_};
    static size_t hung_blocksync{};
    if (request_ && Clock::now() >= request_expiry_) {
      ++hung_blocksync;
      log()->warn("hung blocksync {}", hung_blocksync);
      request_->cancel();
      request_.reset();
    }
    if (request_) {
      return;
    }
    if (requests_.empty()) {
      return;
    }
    auto [peer, tsk]{std::move(requests_.front())};
    requests_.pop();
    if (auto ts{getLocal(tsk)}) {
      thread.io->post([=, peer{std::move(peer)}] { onTs(peer, ts); });
      return;
    }
    uint64_t probable_depth = 100;
    request_expiry_ = Clock::now() + std::chrono::seconds{20};
    request_ = BlocksyncRequest::newRequest(
        *host_,
        *scheduler_,
        ipld_,
        put_block_header_,
        peer,
        tsk.cids(),
        probable_depth,
        blocksync::kBlocksOnly,
        15000,
        [this](auto r) { downloaderCallback(std::move(r)); });
  }

  void SyncJob::downloaderCallback(BlocksyncRequest::Result r) {
    std::unique_lock lock{requests_mutex_};
    if (request_) {
      request_->cancel();
      request_.reset();
    }
    lock.unlock();

    TipsetCPtr ts;
    if (auto _ts{ts_load_->load(r.blocks_available)}) {
      ts = _ts.value();
    } else {
      peers_->onError(*r.from);
      r.delta_rating -= 500;
    }

    if (ts) {
      thread.io->post([this, peer{r.from}, ts] { onTs(peer, ts); });
    }

    fetchDequeue();
  }
}  // namespace fc::sync
