/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "syncer.hpp"

#include "tipset_loader.hpp"
#include "interpret_job.hpp"
#include "subchain_loader.hpp"

namespace fc::sync {

  Syncer::Syncer(std::shared_ptr<libp2p::protocol::Scheduler> scheduler,
                 std::shared_ptr<TipsetLoader> tipset_loader,
                 std::shared_ptr<ChainDb> chain_db,
                 std::shared_ptr<storage::PersistentBufferMap> kv_store,
                 std::shared_ptr<vm::interpreter::Interpreter> interpreter,
                 IpfsStoragePtr ipld)
      : scheduler_(std::move(scheduler)),
        tipset_loader_(std::move(tipset_loader)),
        chain_db_(std::move(chain_db)),
        interpreter_(std::make_shared<InterpretJob>(
            std::move(kv_store),
            std::move(interpreter),
            *scheduler_,
            *chain_db_,
            std::move(ipld),
            [this](const InterpreterJob::Result &result) {
              onInterpreterResult(result);
            })) {}

  void Syncer::start() {
    if (!started_) {
      started_ = true;
      tipset_loader_->init([this](sync::TipsetHash hash,
                                  outcome::result<sync::TipsetCPtr> tipset) {
        onTipsetLoaded(std::move(hash), std::move(tipset));
      });
    }

    if (!isActive()) {
      auto target = chooseNextTarget();
      if (target) {
        auto &t = target.value()->second;
        startJob(target.value()->first, std::move(t.head_tipset), t.height);
        pending_targets_.erase(target.value());
      }
    }
  }

  void Syncer::newTarget(boost::optional<PeerId> peer,
                         TipsetKey head_tipset,
                         BigInt weight,
                         uint64_t height) {
    if (weight < current_weight_ && height < current_height_) {
      // not a sync target
      return;
    }

    if (!peer) {
      if (last_good_peer_) {
        peer = last_good_peer_;
      } else {
        return;
      }
    }

    if (started_ && !isActive()) {
      startJob(std::move(peer.value()), std::move(head_tipset), height);
    } else {
      pending_targets_[peer.value()] =
          Target{std::move(head_tipset), std::move(weight), std::move(height)};
    }
  }

  void Syncer::excludePeer(const PeerId &peer) {
    pending_targets_.erase(peer);
  }

  void Syncer::setCurrentWeightAndHeight(BigInt w, uint64_t h) {
    current_weight_ = std::move(w);
    current_height_ = h;

    for (auto it = pending_targets_.begin(); it != pending_targets_.end();) {
      if (it->second.weight <= current_weight_
          && it->second.height <= current_height_) {
        it = pending_targets_.erase(it);
      } else {
        ++it;
      }
    }
  }

  bool Syncer::isActive() {
    return started_ && current_job_ && current_job_->isActive();
  }

  boost::optional<Syncer::PendingTargets::iterator> Syncer::chooseNextTarget() {
    boost::optional<PendingTargets::iterator> target;

    if (!pending_targets_.empty()) {
      BigInt max_weight = current_weight_;
      Height max_height = current_height_;
      for (auto it = pending_targets_.begin(); it != pending_targets_.end();
           ++it) {
        if (it->second.weight > max_weight) {
          max_weight = it->second.weight;
          target = it;
        } else if (it->second.weight == max_weight
                   && it->second.height > current_height_) {
          max_height = current_height_;
          target = it;
        }
      }
      if (!target) {
        // all targets are obsolete, forget them
        pending_targets_.clear();
      }
    }

    // TODO (artem) choose peer by minimal latency among connected peers with
    // the same weight. Using PeerManager

    return target;
  }

  void Syncer::startJob(PeerId peer, TipsetKey head_tipset, uint64_t height) {
    assert(started_);

    if (!current_job_) {
      current_job_ = std::make_unique<SyncJob>(
          *scheduler_, *tipset_loader_, *chain_db_, [this](SyncStatus status) {
            onSyncJobFinished(std::move(status));
          });
    }

    assert(!current_job_->isActive());

    uint64_t probable_depth = height;
    if (height > probable_height_) {
      probable_depth = height - probable_height_;
    }

    current_job_->start(
        std::move(peer), std::move(head_tipset), probable_depth);
  }

  void Syncer::onTipsetLoaded(TipsetHash hash,
                              outcome::result<TipsetCPtr> tipset_res) {
    if (isActive()) {
      probable_height_ = tipset_res.value()->height();
      current_job_->onTipsetLoaded(std::move(hash), std::move(tipset_res));
    }
  }

  void Syncer::onSyncJobFinished(SyncStatus status) {
    if (status.code == SyncStatus::SYNCED_TO_GENESIS) {
      last_good_peer_ = status.peer.value();
      auto res = interpreter_job_->start(status.head.value());
      if (!res) {
        // ~~log
        // XXX ???? callback_(interpreter_job_->getResult());
      }
    } else {
      // ~~log
    }
  }

  void Syncer::onInterpreterResult(const InterpreterJob::Result& result) {
    if (result.result) {
      callback_(result);
    } else {
      //~~~ ????
    }
  }

}  // namespace fc::sync
