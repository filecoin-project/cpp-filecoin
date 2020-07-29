/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sync_job.hpp"
#include "tipset_loader.hpp"

namespace fc::sync {

  SyncJob::SyncJob(libp2p::protocol::Scheduler &scheduler,
                   TipsetLoader &tipset_loader,
                   ChainDb &chain_db,
                   Callback callback)
      : scheduler_(scheduler),
        tipset_loader_(tipset_loader),
        chain_db_(chain_db),
        callback_(std::move(callback)) {
    assert(callback_);
  }

  void SyncJob::start(PeerId peer, TipsetKey head, uint64_t probable_depth) {
    if (active_) {
      // log~~
      return;
    }
    active_ = true;

    status_.peer = std::move(peer);
    status_.head = std::move(head);

    try {
      if (!chain_db_.tipsetIsStored(head.hash())) {
        // not indexed, loading...
        OUTCOME_EXCEPT(tipset_loader_.loadTipsetAsync(
            status_.head.value(), status_.peer, probable_depth));
        status_.next = status_.head.value().hash();
        status_.code = SyncStatus::IN_PROGRESS;
        return;
      }

      OUTCOME_EXCEPT(maybe_next_target, chain_db_.getUnsyncedBottom(head));

      nextTarget(std::move(maybe_next_target));

    } catch (const std::system_error &e) {
      // log ~~~
      internalError(e.code());
    }
  }

  void SyncJob::cancel() {
    if (active_) {
      SyncStatus s;
      std::swap(s, status_);
      cb_handle_.cancel();
      active_ = false;
    }
  }

  bool SyncJob::isActive() const {
    return active_;
  }

  const SyncStatus &SyncJob::getStatus() const {
    return status_;
  }

  void SyncJob::onTipsetLoaded(
      TipsetHash hash, outcome::result<std::shared_ptr<Tipset>> result) {
    if (status_.code != SyncStatus::IN_PROGRESS || !status_.next.has_value()
        || hash != status_.next.value()) {
      // dont need this tipset
      return;
    }

    try {
      OUTCOME_EXCEPT(tipset, result);
      OUTCOME_EXCEPT(parent_key, tipset->getParents());
      OUTCOME_EXCEPT(maybe_next_target,
                     chain_db_.storeTipset(tipset, parent_key));

      nextTarget(std::move(maybe_next_target));

    } catch (const std::system_error &e) {
      // TODO (artem) separate bad blocks error vs. other errors
      internalError(e.code());
    }
  }

  void SyncJob::internalError(std::error_code e) {
    status_.error = e;
    status_.code = SyncStatus::INTERNAL_ERROR;
    scheduleCallback();
  }

  void SyncJob::scheduleCallback() {
    cb_handle_ = scheduler_.schedule([this]() {
      SyncStatus s;
      std::swap(s, status_);
      active_ = false;
      callback_(s);
    });
  }

  void SyncJob::nextTarget(boost::optional<TipsetCPtr> last_loaded) {
    if (!last_loaded) {
      status_.next = TipsetHash{};
      status_.code = SyncStatus::SYNCED_TO_GENESIS;
      scheduleCallback();
      return;
    }

    auto &roots = last_loaded.value();

    status_.last_loaded = roots->key.hash();
    OUTCOME_EXCEPT(next_key, roots->getParents());
    status_.next = next_key.hash();

    OUTCOME_EXCEPT(tipset_loader_.loadTipsetAsync(
        std::move(next_key), status_.peer, roots->height() - 1));
  }

  Syncer::Syncer(std::shared_ptr<libp2p::protocol::Scheduler> scheduler,
                 std::shared_ptr<TipsetLoader> tipset_loader,
                 std::shared_ptr<ChainDb> chain_db,
                 Callback callback)
      : scheduler_(std::move(scheduler)),
        tipset_loader_(std::move(tipset_loader)),
        chain_db_(std::move(chain_db)),
        callback_(std::move(callback)) {}

  void Syncer::start() {
    if (!started_) {
      started_ = true;
      tipset_loader_->init([this](sync::TipsetHash hash,
                                 outcome::result<fc::sync::Tipset> tipset) {
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

  void Syncer::newTarget(PeerId peer,
                         TipsetKey head_tipset,
                         BigInt weight,
                         uint64_t height) {
    if (weight <= current_weight_) {
      // not a sync target
      return;
    }

    if (started_ && !isActive()) {
      startJob(std::move(peer), std::move(head_tipset), height);
    } else {
      pending_targets_[peer] =
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
      if (it->second.weight <= current_weight_) {
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
      for (auto it = pending_targets_.begin(); it != pending_targets_.end();
           ++it) {
        if (it->second.weight > max_weight) {
          max_weight = it->second.weight;
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
    if (height > current_height_) {
      probable_depth = height - current_height_;
    }

    current_job_->start(
        std::move(peer), std::move(head_tipset), probable_depth);
  }

  void Syncer::onTipsetLoaded(TipsetHash hash,
                              outcome::result<Tipset> tipset_res) {
    if (isActive()) {
      if (tipset_res) {
        current_job_->onTipsetLoaded(
            std::move(hash),
            std::make_shared<Tipset>(std::move(tipset_res.value())));
      } else {
        current_job_->onTipsetLoaded(std::move(hash), tipset_res.error());
      }
    }
  }

  void Syncer::onSyncJobFinished(SyncStatus status) {
    callback_(std::move(status));
  }

}  // namespace fc::sync
