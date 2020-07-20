/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_JOB_HPP
#define CPP_FILECOIN_SYNC_JOB_HPP

#include <libp2p/protocol/common/scheduler.hpp>
#include "common.hpp"
#include "storage/indexdb/indexdb.hpp"

namespace fc::sync {

  class TipsetLoader;

  using storage::indexdb::BranchId;

  struct SyncStatus {
    enum StatusCode {
      IDLE,
      IN_PROGRESS,
      SYNCED_TO_GENESIS,
      INTERRUPTED,
      BAD_BLOCKS,
      INTERNAL_ERROR,
    };

    StatusCode code = IDLE;
    std::error_code error;
    boost::optional<PeerId> peer;
    boost::optional<TipsetKey> head;
    boost::optional<TipsetHash> last_loaded;
    boost::optional<TipsetHash> next;
    BranchId branch = storage::indexdb::kNoBranch;
    uint64_t total = 0;
  };

  class SyncJob {
   public:
    using Callback = std::function<void(SyncStatus status)>;

    SyncJob(libp2p::protocol::Scheduler &scheduler,
            TipsetLoader &tipset_loader,
            storage::indexdb::IndexDb &index_db,
            Callback callback);

    void start(PeerId peer, TipsetKey head, uint64_t probable_depth);

    void cancel();

    bool isActive() const;

    const SyncStatus &getStatus() const;

    void onTipsetLoaded(TipsetHash hash, outcome::result<Tipset> result);

   private:
    void internalError(std::error_code e);

    void scheduleCallback();

    libp2p::protocol::Scheduler &scheduler_;
    TipsetLoader &tipset_loader_;
    storage::indexdb::IndexDb &index_db_;
    bool active_ = false;
    SyncStatus status_;
    Callback callback_;
    libp2p::protocol::scheduler::Handle cb_handle_;
  };

  class Syncer {
   public:
    using Callback = std::function<void(SyncStatus status)>;

    void start();

    void newTarget(PeerId peer,
                   TipsetKey head_tipset,
                   BigInt weight,
                   uint64_t height);

    void excludePeer(const PeerId &peer);

    void setCurrentWeightAndHeight(BigInt w, uint64_t h);

    bool isActive();

   private:
    struct Target {
      TipsetKey head_tipset;
      BigInt weight;
      uint64_t height;
    };

    using PendingTargets = std::unordered_map<PeerId, Target>;

    boost::optional<PendingTargets::iterator> chooseNextTarget();

    void startJob(PeerId peer, TipsetKey head_tipset, uint64_t height);

    void onTipsetLoaded(TipsetHash hash, outcome::result<Tipset> tipset);

    void onSyncJobFinished(SyncStatus status);

    std::shared_ptr<libp2p::protocol::Scheduler> scheduler_;
    std::shared_ptr<TipsetLoader> tipset_loader_;
    std::shared_ptr<storage::indexdb::IndexDb> index_db_;
    Callback callback_;

    PendingTargets pending_targets_;

    // max weight of local node
    BigInt current_weight_;

    // height of local node
    uint64_t current_height_ = 0;

    // one job at the moment, they could be parallel
    std::unique_ptr<SyncJob> current_job_;
    bool started_ = false;
  };

}  // namespace fc::sync

#endif  // CPP_FILECOIN_SYNC_JOB_HPP
