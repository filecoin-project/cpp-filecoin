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

        PairStorage(const CommitCallback &commit_callback,
                    const AggregateInput &aggregate_input);
      };

      void push(const SectorNumber &sector_number,
                const AggregateInput &aggregate_input,
                const CommitCallback &commit_callback);
      void push(const SectorNumber &sector_number,
                const CommitCallback &commit_callback,
                const AggregateInput &aggregate_input);
      void push(const SectorNumber &sector_number,
                const PairStorage &pair_storage); // TODO make one push

      size_t size() const;

      std::map<SectorNumber, PairStorage>::iterator begin();
      std::map<SectorNumber, PairStorage>::iterator end();

     private:
      std::mutex mutex_;

      std::map<SectorNumber, PairStorage> storage_;
    };

    CommitBatcherImpl(const std::chrono::milliseconds &max_time,
                      const size_t &max_size_callback,
                      const std::shared_ptr<Scheduler> &scheduler);

    /*
     * 1) Должен уметь отправлять информацию за время, которую ему задали
     * NEW 2) Должен уметь отправлять информацию за размер, которую ему задали
     * (например 10 коммитов макс вместимость мы должны его отправлять)
     * 3) После любой отправки время отправки батчера заводится снова (
     * скедлинг, хэндлер)
     */
    outcome::result<void> addCommit(const SectorInfo &sector_info,
                                    const AggregateInput &aggregate_input,
                                    const CommitCallback &callBack);

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

    void sendCallbacks();

    outcome::result<CID> sendBatch();

  };

}  // namespace fc::mining