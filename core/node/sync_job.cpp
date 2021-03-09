/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sync_job.hpp"

#include "blocksync_common.hpp"
#include "common/logger.hpp"
#include "common/outcome2.hpp"
#include "events.hpp"
#include "node/chain_store_impl.hpp"
#include "vm/interpreter/interpreter.hpp"

namespace fc::sync {
  using primitives::tipset::chain::stepParent;

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("sync_job");
      return logger.get();
    }
  }  // namespace

  outcome::result<TipsetCPtr> stepUp(TsLoadPtr ts_load,
                                     TsBranchPtr branch,
                                     TipsetCPtr ts) {
    if (branch->chain.rbegin()->second.key != ts->key) {
      OUTCOME_TRY(it, find(branch, ts->height() + 1, false));
      OUTCOME_TRY(it2, stepParent(it));
      if (it2.second->second.key == ts->key) {
        return ts_load->loadw(it.second->second);
      }
    }
    return OutcomeError::kDefault;
  }

  SyncJob::SyncJob(std::shared_ptr<libp2p::Host> host,
                   std::shared_ptr<ChainStoreImpl> chain_store,
                   std::shared_ptr<libp2p::protocol::Scheduler> scheduler,
                   std::shared_ptr<Interpreter> interpreter,
                   std::shared_ptr<InterpreterCache> interpreter_cache,
                   SharedMutexPtr ts_branches_mutex,
                   TsBranchesPtr ts_branches,
                   KvPtr ts_main_kv,
                   TsBranchPtr ts_main,
                   TsLoadPtr ts_load,
                   IpldPtr ipld)
      : host_(std::move(host)),
        chain_store_(std::move(chain_store)),
        scheduler_(std::move(scheduler)),
        interpreter_(std::move(interpreter)),
        interpreter_cache_(std::move(interpreter_cache)),
        ts_branches_mutex_{std::move(ts_branches_mutex)},
        ts_branches_{std::move(ts_branches)},
        ts_main_kv_(std::move(ts_main_kv)),
        ts_main_(std::move(ts_main)),
        ts_load_(std::move(ts_load)),
        ipld_(std::move(ipld)) {
    attached_.insert(ts_main_);
  }

  void SyncJob::start(std::shared_ptr<events::Events> events) {
    if (events_) {
      log()->error("already started");
      return;
    }

    events_ = std::move(events);
    assert(events_);

    possible_head_event_ = events_->subscribePossibleHead(
        [this](const events::PossibleHead &e) { onPossibleHead(e); });

    peers_.start(
        host_, *events_, [](const std::set<std::string> &protocols) -> bool {
          static const std::string id(blocksync::kProtocolId);
          return (protocols.count(id) > 0);
        });

    log()->debug("started");
  }

  void SyncJob::onPossibleHead(const events::PossibleHead &e) {
    if (auto ts{getLocal(e.head)}) {
      thread.io->post([this, peer{e.source}, ts] { onTs(peer, ts); });
    } else if (e.source) {
      fetch(*e.source, e.head);
    }
  }

  TipsetCPtr SyncJob::getLocal(const TipsetKey &tsk) {
    if (auto _ts{ts_load_->load(tsk)}) {
      auto &ts{_ts.value()};
      for (auto &block : ts->blks) {
        if (auto _has{ipld_->contains(block.messages)};
            !_has || !_has.value()) {
          return nullptr;
        }
      }
      return ts;
    }
    return nullptr;
  }

  void SyncJob::onTs(const boost::optional<PeerId> &peer, TipsetCPtr ts) {
    std::unique_lock lock{*ts_branches_mutex_};
    while (true) {
      std::vector<TsBranchPtr> children;
      auto branch{insert(*ts_branches_, ts, &children).first};
      if (attached_.count(branch)) {
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
            continue;
          }
          if (peer) {
            thread.io->post([this, peer{*peer}, tsk{*branch->parent_key}] {
              fetch(peer, tsk);
            });
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
      if (auto _ts{ts_load_->loadw(branch->chain.rbegin()->second)}) {
        auto &ts{_ts.value()};
        if (ts->getParentWeight() > attached_heaviest_.second) {
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

  void SyncJob::updateTarget(TsBranchPtr last) {
    auto branch{attached_heaviest_.first};
    if (branch == last) {
      return;
    }
    auto it{std::prev(branch->chain.end())};
    while (true) {
      if (auto _res{interpreter_cache_->tryGet(it->second.key)}) {
        if (*_res) {
          if (auto _ts{ts_load_->loadw(it->second)}) {
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

  void SyncJob::onInterpret(TipsetCPtr ts, const InterpreterResult &result) {
    auto &weight{result.weight};
    if (weight > chain_store_->getHeaviestWeight()) {
      if (auto _update{
              update(ts_main_, find(*ts_branches_, ts), ts_main_kv_)}) {
        auto &[path, removed]{_update.value()};
        for (auto &branch : removed) {
          ts_branches_->erase(branch);
          attached_.erase(branch);
        }
        chain_store_->update(path, weight);
      } else {
        log()->error("update {:#}", _update.error());
      }
    }
  }

  bool SyncJob::checkParent(TipsetCPtr ts) {
    if (ts->height() != 0) {
      if (auto _res{interpreter_cache_->tryGet(ts->getParents())}) {
        if (*_res) {
          auto &res{_res->value()};
          if (ts->getParentStateRoot() != res.state_root) {
            log()->warn("parent state mismatch {} {}",
                        ts->height(),
                        fmt::join(ts->key.cids(), ","));
            return false;
          }
          if (ts->getParentMessageReceipts() != res.message_receipts) {
            log()->warn("parent receipts mismatch {} {}",
                        ts->height(),
                        fmt::join(ts->key.cids(), ","));
            return false;
          }
        } else {
          log()->warn("parent interpret error {} {}",
                      ts->height(),
                      fmt::join(ts->key.cids(), ","));
          return false;
        }
      } else {
        log()->warn("parent not interpreted {} {}",
                    ts->height(),
                    fmt::join(ts->key.cids(), ","));
        return false;
      }
      if (auto _has{ipld_->contains(ts->getParentStateRoot())};
          !_has || !_has.value()) {
        log()->warn("no parent state {} {}",
                    ts->height(),
                    fmt::join(ts->key.cids(), ","));
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
    auto branch{find(*ts_branches_, ts).first};
    if (!checkParent(ts)) {
      // TODO: detach and ban branches
      interpret_ts_ = nullptr;
      return;
    }
    interpreting_ = true;
    interpret_thread.io->post([=] {
      auto result{interpreter_->interpret(branch, ts)};
      if (!result) {
        log()->warn("interpret error {:#}", result.error());
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
            // TODO: detach and ban branches
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
    fetchDequeue();
  }

  void SyncJob::fetchDequeue() {
    std::lock_guard lock{requests_mutex_};
    if (request_) {
      return;
    }
    if (requests_.empty()) {
      return;
    }
    auto [peer, tsk]{std::move(requests_.back())};
    requests_.pop();
    if (auto ts{getLocal(tsk)}) {
      onTs(peer, ts);
      return;
    }
    uint64_t probable_depth = 5;
    request_ = BlocksyncRequest::newRequest(
        *host_,
        *scheduler_,
        *ipld_,
        peer,
        tsk.cids(),
        probable_depth,
        blocksync::BLOCKS_AND_MESSAGES,
        60000,
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
      r.delta_rating -= 500;
    }

    if (r.from && r.delta_rating != 0) {
      peers_.changeRating(r.from.value(), r.delta_rating);
    }

    if (ts) {
      thread.io->post([this, peer{r.from}, ts] { onTs(peer, ts); });
    }

    fetchDequeue();
  }
}  // namespace fc::sync
