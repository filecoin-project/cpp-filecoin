/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "syncer.hpp"

#include "common/logger.hpp"
#include "tipset_loader.hpp"

namespace fc::sync {

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("syncer");
      return logger.get();
    }
  }  // namespace

  Syncer::Syncer(std::shared_ptr<libp2p::protocol::Scheduler> scheduler,
                 std::shared_ptr<TipsetLoader> tipset_loader,
                 std::shared_ptr<ChainDb> chain_db,
                 std::shared_ptr<storage::PersistentBufferMap> kv_store,
                 std::shared_ptr<vm::interpreter::Interpreter> interpreter,
                 IpfsStoragePtr ipld)
      : scheduler_(std::move(scheduler)),
        tipset_loader_(std::move(tipset_loader)),
        chain_db_(std::move(chain_db)),
        downloader_(*scheduler_,
                    *tipset_loader_,
                    *chain_db_,
                    [this](SubchainLoader::Status status) {
                      downloaderCallback(std::move(status));
                    }),
        interpreter_(std::move(kv_store),
                     std::move(interpreter),
                     *scheduler_,
                     *chain_db_,
                     std::move(ipld),
                     [this](InterpretJob::Result result) {
                       interpreterCallback(std::move(result));
                     }) {}

  void Syncer::start(std::shared_ptr<events::Events> events) {
    if (events_) {
      log()->error("already started");
      return;
    }

    events_ = std::move(events);
    assert(events_);

    possible_head_event_ = events_->subscribePossibleHead(
        [this](const events::PossibleHead &e) { onPossibleHead(e); });

    tipset_stored_event_ =
        events_->subscribeTipsetStored([this](const events::TipsetStored &e) {
          if (e.tipset.has_value()) {
            if (e.tipset.value()->height() > last_known_height_) {
              last_known_height_ = e.tipset.value()->height();
            }
          }
          downloader_.onTipsetStored(e);
        });

    peer_disconnected_event_ = events_->subscribePeerDisconnected(
        [this](const events::PeerDisconnected &e) {
          pending_targets_.erase(e.peer_id);
        });

    peers_.start({"/blocksync/"}, *events);
  }

  void Syncer::downloaderCallback(SubchainLoader::Status status) {
    if (status.code == SubchainLoader::Status::SYNCED_TO_GENESIS) {
      peers_.changeRating(status.peer.value(), 100);
      newInterpretJob(std::move(status.head.value()));
    } else {
      // TODO (maybe restart with another peer)
      peers_.changeRating(status.peer.value(), -100);
    }

    while (!pending_targets_.empty() && !downloader_.isActive()) {
      auto it = pending_targets_.begin();
      auto res = chain_db_->getUnsyncedBottom(it->second.head_tipset);
      if (res) {
        auto &lowest_loaded = res.value();
        if (!lowest_loaded) {
          newInterpretJob(std::move(it->second.head_tipset));
        } else {
          onPossibleHead({.source = it->first,
                          .head = lowest_loaded.value()->getParents(),
                          .height = lowest_loaded.value()->height()});
        }
      } else {
        onPossibleHead({.source = it->first,
                        .head = it->second.head_tipset,
                        .height = it->second.height});
      }
      pending_targets_.erase(it);
    }
  }

  void Syncer::interpreterCallback(InterpretJob::Result result) {
    events_->signalHeadInterpreted(std::move(result));
    if (!pending_interpret_targets_.empty()) {
      auto target = std::move(pending_interpret_targets_.front());
      pending_interpret_targets_.pop_front();
      newInterpretJob(std::move(target));
    }
  }

  void Syncer::newInterpretJob(TipsetKey key) {
    if (interpreter_.getStatus().active) {
      pending_interpret_targets_.push_back(std::move(key));
    } else {
      auto res = interpreter_.start(std::move(key));
      if (res) {
        if (res.value().has_value()) {
          interpreterCallback(std::move(res.value().value()));
        }
      } else {
        log()->error("interpreter start error {}", res.error().message());
      }
    }
  }

  void Syncer::onPossibleHead(const events::PossibleHead &e) {
    auto maybe_peer = choosePeer(e.source);
    if (!maybe_peer) {
      log()->debug("ignoring sync target, no peers connected");
      return;
    }

    PeerId &peer = maybe_peer.value();
    if (downloader_.isActive()) {
      auto it = pending_targets_.find(peer);
      if (it == pending_targets_.end()) {
        pending_targets_[peer] = DownloadTarget{e.head, e.height};
      } else if (it->second.height <= e.height
                 && it->second.head_tipset != e.head) {
        // prefer more fresh update
        it->second = DownloadTarget{e.head, e.height};
      }
    } else {
      // if e.source is null then try to make the 1st request with depth
      newDownloadJob(std::move(peer), e.head, e.height, !e.source.has_value());
    }
  }

  boost::optional<PeerId> Syncer::choosePeer(
      boost::optional<PeerId> candidate) {
    if (candidate && peers_.isConnected(candidate.value())) {
      return candidate;
    }
    return peers_.selectBestPeer();
  }

  void Syncer::newDownloadJob(PeerId peer,
                              TipsetKey head,
                              Height height,
                              bool make_deep_request) {
    assert(!downloader_.isActive());

    uint64_t probable_depth = 1;
    if (make_deep_request) {
      // if e.source is null then try to make the 1st request with depth
      if (height > last_known_height_) {
        probable_depth = height - last_known_height_;
      }
    }
    downloader_.start(std::move(peer), std::move(head), probable_depth);
  }

}  // namespace fc::sync
