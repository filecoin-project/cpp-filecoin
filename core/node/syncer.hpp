/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_SYNCER_HPP
#define CPP_FILECOIN_SYNC_SYNCER_HPP

#include <libp2p/protocol/common/scheduler.hpp>
#include "interpret_job.hpp"

namespace fc::sync {

  class TipsetLoader;
  class SubchainLoader;
  class InterpretJob;

  class Syncer {
   public:
    Syncer(std::shared_ptr<libp2p::protocol::Scheduler> scheduler,
           std::shared_ptr<TipsetLoader> tipset_loader,
           std::shared_ptr<ChainDb> chain_db,
           std::shared_ptr<storage::PersistentBufferMap> kv_store,
           std::shared_ptr<vm::interpreter::Interpreter> interpreter,
           IpfsStoragePtr ipld);

    void start(std::shared_ptr<events::Events> events);

   private:
    void newTarget(boost::optional<PeerId> peer,
                   TipsetKey head_tipset,
                   BigInt weight,
                   uint64_t height);

    void excludePeer(const PeerId &peer);

    void setCurrentWeightAndHeight(BigInt w, uint64_t h);

    bool isActive();

    struct Target {
      TipsetKey head_tipset;
      BigInt weight;
      uint64_t height;
    };

    using PendingTargets = std::unordered_map<PeerId, Target>;

    boost::optional<PendingTargets::iterator> chooseNextTarget();

    void startJob(PeerId peer, TipsetKey head_tipset, uint64_t height);

    std::shared_ptr<libp2p::protocol::Scheduler> scheduler_;
    std::shared_ptr<TipsetLoader> tipset_loader_;
    std::shared_ptr<ChainDb> chain_db_;

    PendingTargets pending_targets_;

    // max weight of local node
    BigInt current_weight_;

    // height of local node
    uint64_t current_height_ = 0;

    boost::optional<PeerId> last_good_peer_;
    Height probable_height_ = 0;

    // one job at the moment, they could be parallel
    std::unique_ptr<SubchainLoader> downloader_;
    std::shared_ptr<InterpretJob> interpreter_;

    bool started_ = false;

    std::shared_ptr<events::Events> events_;
  };

}  // namespace fc::sync

#endif  // CPP_FILECOIN_SYNC_SYNCER_HPP
