/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include <libp2p/basic/scheduler.hpp>
#include "miner/storage_fsm/commit_batcher.hpp"

namespace fc::mining {

  using libp2p::basic::Scheduler;
  using primitives::SectorNumber;
  using primitives::address::Address;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetKey;
  using proofs::ProofEngine;
  using types::FeeConfig;

  class CommitBatcherImpl : public CommitBatcher {
   public:
    struct PairStorage {
      AggregateInput aggregate_input;
      CommitCallback commit_callback;
    };
    typedef std::map<SectorNumber, PairStorage> MapPairStorage;

    CommitBatcherImpl(const std::chrono::milliseconds &max_time,
                      const size_t &max_size_callback,
                      const std::shared_ptr<Scheduler> &scheduler);

    outcome::result<void> addCommit(const SectorInfo &sector_info,
                                    const AggregateInput &aggregate_input,
                                    const CommitCallback &callBack) override;

    void forceSend() override;

    void setCommitCutoff(const ChainEpoch &current_epoch,
                         const SectorInfo &sector_info);

   private:
    std::shared_ptr<Scheduler> scheduler_;
    Scheduler::Handle handle_;
    std::chrono::milliseconds max_delay_;
    std::chrono::milliseconds closest_cutoff_;
    std::chrono::system_clock::time_point cutoff_start_;
    size_t max_size_callback_;
    MapPairStorage union_storage_;
    std::shared_ptr<FullNodeApi> api_;
    Address miner_address_;
    std::shared_ptr<FeeConfig> fee_config_;
    std::shared_ptr<ProofEngine> proof_;
    std::mutex mutex_storage_;

    void sendCallbacks();

    outcome::result<CID> sendBatch(MapPairStorage &union_storage_for_send);

    outcome::result<TokenAmount> getSectorCollateral(
        const SectorNumber &sector_number, const TipsetKey &tip_set_key);
  };

}  // namespace fc::mining