/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sync_job.hpp"

#include "blocksync_common.hpp"
#include "chain_db.hpp"
#include "common/logger.hpp"
#include "events.hpp"

namespace fc::sync {

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("sync_job");
      return logger.get();
    }
  }  // namespace

  SyncJob::SyncJob(std::shared_ptr<ChainDb> chain_db,
                   std::shared_ptr<libp2p::Host> host,
                   std::shared_ptr<libp2p::protocol::Scheduler> scheduler,
                   IpldPtr ipld)
      : chain_db_(std::move(chain_db)),
        host_(std::move(host)),
        scheduler_(std::move(scheduler)),
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
    DownloadTarget target{.head = e.head, .height = e.height};
    if (e.source) {
      target.peers.insert(e.source.value());
    }

    if (!adjustTarget(target)) {
      return;
    }

    if (now_active_) {
      enqueue(std::move(target));
    } else {
      // try to make the 1st request with depth=1
      newJob(std::move(target), e.head != target.head);
    }
  }

  bool SyncJob::adjustTarget(DownloadTarget &target) {
    auto state_res = chain_db_->getSyncState(target.head.hash());
    if (!state_res) {
      log()->critical("chain db inconsistency");
      events_->signalFatalError({.message = state_res.error().message()});
      return false;
    }

    auto &state = state_res.value();

    if (state.chain_indexed) {
      log()->debug(
          "chain from target tipset (h={}) is already in sync state, ignoring "
          "target",
          target.height);
      return false;
    }

    if (!target.active_peer
        || !peers_.isConnected(target.active_peer.value())) {
      target.active_peer = peers_.selectBestPeer(target.peers);
    }

    if (!target.active_peer) {
      log()->debug("no peers connected, ignoring target");
      return false;
    }

    if (state.tipset_indexed) {
      assert(state.unsynced_bottom);
      assert(state.unsynced_bottom->height() > 1);

      target.head = state.unsynced_bottom->getParents();
      auto new_height = state.unsynced_bottom->height() - 1;
      log()->debug("tipset (h={}) already indexed, changing target to h<={}",
                   target.height,
                   new_height);
      target.height = new_height;
    }

    return true;
  }

  void SyncJob::enqueue(DownloadTarget target) {
    if (now_active_ && now_active_->height == target.height
        && now_active_->head == target.head) {
      if (!target.peers.empty()) {
        now_active_->peers.insert(target.peers.begin(), target.peers.end());
      }
      return;
    }

    std::pair<Height, TipsetHash> k{target.height, target.head.hash()};
    auto it = pending_targets_.find(k);

    if (it == pending_targets_.end()) {
      target.active_peer.reset();
      pending_targets_.insert({std::move(k), std::move(target)});
    } else {
      it->second.peers.insert(target.peers.begin(), target.peers.end());
    }
  }

  void SyncJob::newJob(DownloadTarget target, bool make_deep_request) {
    assert(!now_active_);
    assert(target.active_peer);

    uint64_t probable_depth = 1;
    if (make_deep_request) {
      // if e.source is null then try to make the 1st request with depth
      if (target.height > last_known_height_) {
        probable_depth = target.height - last_known_height_;
      } else {
        probable_depth = target.height - 1;
      }
    }

    request_.emplace(
        *chain_db_,
        *host_,
        *scheduler_,
        *ipld_,
        target.active_peer.value(),
        target.head.cids(),
        probable_depth,
        30000,
        true,  // TODO index head tipset (?)
        [this](TipsetRequest::Result r) { downloaderCallback(std::move(r)); });

    now_active_ = std::move(target);
  }

  void SyncJob::downloaderCallback(TipsetRequest::Result r) {
    request_.reset();

    if (r.from && r.delta_rating != 0) {
      peers_.changeRating(r.from.value(), r.delta_rating);
    }

    auto target = std::move(now_active_);
    now_active_.reset();

    if (r.head) {
      if (last_known_height_ < r.head->height()) {
        last_known_height_ = r.head->height();
      }
    }

    if (r.next_target) {
      target->height = r.next_target_height;
      target->head = std::move(r.next_target.value());
      if (r.from && r.delta_rating > 0) {
        // something was downloaded, better continue with this peer
        target->active_peer = std::move(r.from);
        newJob(std::move(target.value()), true);
        return;

      } else {
        enqueue(std::move(target.value()));
      }
    }

    auto new_target = dequeue();
    if (new_target) {
       newJob(new_target.value(), false);
    }
  }

  boost::optional<SyncJob::DownloadTarget> SyncJob::dequeue() {
    boost::optional<DownloadTarget> target = boost::none;

    while (!pending_targets_.empty()) {
      auto begin = pending_targets_.begin();
      auto candidate = std::move(begin->second);
      pending_targets_.erase(begin);

      if (adjustTarget(candidate)) {
        target = std::move(candidate);
        break;
      }
    }

    return target;
  }

}  // namespace fc::sync
