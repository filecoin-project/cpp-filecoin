/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sync_job.hpp"

#include "blocksync_common.hpp"
#include "common/logger.hpp"
#include "events.hpp"
#include "primitives/tipset/chain.hpp"

namespace fc::sync {

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("sync_job");
      return logger.get();
    }
  }  // namespace

  SyncJob::SyncJob(std::shared_ptr<libp2p::Host> host,
                   std::shared_ptr<libp2p::protocol::Scheduler> scheduler,
                   std::shared_ptr<InterpretJob> interpret_job,
                   TsBranches &ts_branches,
                   TsBranchPtr ts_main,
                   TsLoadPtr ts_load,
                   IpldPtr ipld)
      : host_(std::move(host)),
        scheduler_(std::move(scheduler)),
        interpret_job_(std::move(interpret_job)),
        ts_branches_{ts_branches},
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

    peers_.start(
        host_, *events_, [](const std::set<std::string> &protocols) -> bool {
          static const std::string id(blocksync::kProtocolId);
          return (protocols.count(id) > 0);
        });

    log()->debug("started");
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
      auto &ts{_ts.value()};
      for (auto &block : ts->blks) {
        if (!ipld_->contains(block.messages)) {
          return nullptr;
        }
      }
      return ts;
    }
    return nullptr;
  }

  void SyncJob::onTs(const boost::optional<PeerId> &peer, TipsetCPtr ts) {
    std::lock_guard lock{branches_mutex_};
    while (true) {
      auto branch{insert(ts_branches_, ts).first};
      if (branch == ts_main_) {
        spdlog::error("TODO: attached to main branch at {}", ts->height());
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
            fetch(*peer, *branch->parent_key);
          }
        }
      }
      break;
    }
    // TODO: interpret
  }

  void SyncJob::fetch(const PeerId &peer, const TipsetKey &tsk) {
    std::unique_lock lock{requests_mutex_};
    requests_.emplace(peer, tsk);
    lock.unlock();
    fetchDequeue();
  }

  void SyncJob::fetchDequeue() {
    if (request_) {
      return;
    }
    std::unique_lock lock{requests_mutex_};
    auto [peer, tsk]{std::move(requests_.back())};
    requests_.pop();
    uint64_t probable_depth = 5;
    request_ = TipsetRequest::newRequest(
        *host_,
        *scheduler_,
        *ipld_,
        peer,
        tsk.cids(),
        probable_depth,
        60000,
        true,  // TODO index head tipset (?)
        true,
        [this](TipsetRequest::Result r) { downloaderCallback(std::move(r)); });
  }

  void SyncJob::downloaderCallback(TipsetRequest::Result r) {
    if (request_) {
      request_->cancel();
      request_.reset();
    }

    if (r.from && r.delta_rating != 0) {
      peers_.changeRating(r.from.value(), r.delta_rating);
    }

    if (r.head) {
      onTs(r.from, r.head);
    }

    fetchDequeue();
  }
}  // namespace fc::sync
