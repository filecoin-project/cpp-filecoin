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

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("sync_job");
      return logger.get();
    }
  }  // namespace

  SyncJob::SyncJob(std::shared_ptr<libp2p::Host> host,
                   std::shared_ptr<ChainStoreImpl> chain_store,
                   std::shared_ptr<libp2p::protocol::Scheduler> scheduler,
                   std::shared_ptr<InterpretJob> interpret_job,
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
        interpret_job_(std::move(interpret_job)),
        interpreter_cache_(std::move(interpreter_cache)),
        ts_branches_mutex_{std::move(ts_branches_mutex)},
        ts_branches_{std::move(ts_branches)},
        ts_main_kv_(std::move(ts_main_kv)),
        ts_main_(std::move(ts_main)),
        ts_load_(std::move(ts_load)),
        ipld_(std::move(ipld)) {}

  void SyncJob::start(std::shared_ptr<events::Events> events) {
    if (events_) {
      log()->error("already started");
      return;
    }

    events_ = std::move(events);
    assert(events_);

    possible_head_event_ = events_->subscribePossibleHead(
        [this](const events::PossibleHead &e) { onPossibleHead(e); });

    head_interpreted_event_ = events_->subscribeHeadInterpreted(
        [this](const events::HeadInterpreted &e) {
          thread.io->post([this, e] {
            std::unique_lock lock{*ts_branches_mutex_};
            interpret_queue_.push(e.head);
            interpretDequeue();
          });
        });

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
      auto branch{insert(*ts_branches_, ts).first};
      if (branch == ts_main_) {
        interpret_queue_.push(ts);
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

  void SyncJob::interpretDequeue() {
    std::pair<TipsetCPtr, BigInt> heaviest;
    while (!interpret_queue_.empty()) {
      auto ts{interpret_queue_.front()};
      interpret_queue_.pop();
      if (auto _res{interpreter_cache_->tryGet(ts->key)}) {
        if (*_res) {
          auto &res{_res->value()};
          if (!heaviest.first || res.weight > heaviest.second) {
            heaviest = {ts, res.weight};
          }
          auto it{find(*ts_branches_, ts)};
          for (auto it2 : children(it)) {
            if (auto _ts{ts_load_->loadw(it2.second->second)}) {
              auto &ts{_ts.value()};
              if (ts->getParentStateRoot() != res.state_root) {
                log()->warn("parent state mismatch {} {}",
                            ts->height(),
                            fmt::join(ts->key.cids(), ","));
                continue;
              }
              if (ts->getParentMessageReceipts() != res.message_receipts) {
                log()->warn("parent receipts mismatch {} {}",
                            ts->height(),
                            fmt::join(ts->key.cids(), ","));
                continue;
              }
              interpret_queue_.push(ts);
            }
          }
        }
      } else {
        if (auto _has{ipld_->contains(ts->getParentStateRoot())};
            !_has || !_has.value()) {
          log()->warn("no parent state {} {}",
                      ts->height(),
                      fmt::join(ts->key.cids(), ","));
          continue;
        }
        interpret_job_->add(ts);
      }
    }
    if (heaviest.first && heaviest.second > chain_store_->getHeaviestWeight()) {
      if (auto _path{update(
              ts_main_, find(*ts_branches_, heaviest.first), ts_main_kv_)}) {
        auto &[path, removed]{_path.value()};
        for (auto &branch : removed) {
          ts_branches_->erase(branch);
        }
        chain_store_->update(path, heaviest.second);
      } else {
        log()->error("update {:#}", _path.error());
      }
    }
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
