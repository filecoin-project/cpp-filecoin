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
  using types::FeeConfig;
  using proofs::ProofEngine;

  class CommitBatcherImpl : public CommitBatcher {
   public:
    class UnionStorage {
     public:
      UnionStorage() = default;
      UnionStorage(UnionStorage &&);

      UnionStorage(const UnionStorage &) = delete;

      ~UnionStorage() = default;

      struct PairStorage {
        AggregateInput aggregate_input;
        CommitCallback commit_callback;

        PairStorage(const AggregateInput &aggregate_input,
                    const CommitCallback &commit_callback);
      };

      void push(const SectorNumber &sector_number,
                const PairStorage &pair_storage);

      size_t size();

      PairStorage get(const int index);

      std::map<SectorNumber, PairStorage>::iterator begin();
      std::map<SectorNumber, PairStorage>::iterator end();

     private:
      std::mutex mutex_;

      std::map<SectorNumber, PairStorage> storage_;
    }; // TODO вынести PairStorage и удалить UnionStorage. storage_;

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
    UnionStorage union_storage_;
    std::shared_ptr<FullNodeApi> api_;
    Address miner_address_;
    std::shared_ptr<FeeConfig> fee_config_;
    std::shared_ptr<ProofEngine> proof_;

    void sendCallbacks();

    outcome::result<CID> sendBatch(UnionStorage &union_storage_for_send);

    TokenAmount getSectorCollateral(std::shared_ptr<const Tipset> &head,
                                    const SectorNumber &sector_number,
                                    const TipsetKey &tip_set_key);
  };

}  // namespace fc::mining