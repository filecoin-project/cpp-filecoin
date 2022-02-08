/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include <libp2p/basic/scheduler.hpp>
#include "miner/storage_fsm/commit_batcher.hpp"

namespace fc::mining {
  using api::MinerInfo;
  using libp2p::basic::Scheduler;
  using primitives::BigInt;
  using primitives::SectorNumber;
  using primitives::address::Address;
  using primitives::sector::RegisteredAggregationProof;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetKey;
  using proofs::ProofEngine;
  using types::FeeConfig;

  using AddressSelector = std::function<outcome::result<Address>(
      const MinerInfo &miner_info,
      const TokenAmount &good_funds,
      const TokenAmount &need_funds,
      const std::shared_ptr<FullNodeApi> &api)>;

  class CommitBatcherImpl : public CommitBatcher {
   public:
    struct PairStorage {
      AggregateInput aggregate_input;
      CommitCallback commit_callback;
    };
    using MapPairStorage = std::map<SectorNumber, PairStorage>;

    CommitBatcherImpl(const std::chrono::seconds &max_time,
                      std::shared_ptr<FullNodeApi> api,
                      Address miner_address,
                      std::shared_ptr<Scheduler> scheduler,
                      AddressSelector address_selector,
                      std::shared_ptr<FeeConfig> fee_config,
                      const size_t &max_size_callback,
                      std::shared_ptr<ProofEngine> proof);

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
    MapPairStorage pair_storage_;
    std::shared_ptr<FullNodeApi> api_;
    Address miner_address_;
    std::shared_ptr<FeeConfig> fee_config_;
    std::shared_ptr<ProofEngine> proof_;
    std::mutex mutex_storage_;
    AddressSelector address_selector_;
    common::Logger logger_;

    const BigInt agg_fee_num_ = BigInt(110);
    const BigInt agg_fee_den_ = BigInt(100);
    const RegisteredAggregationProof arp_ = RegisteredAggregationProof(0);

    // TODO (Markuu-s) Add processIndividually
    void reschedule(std::chrono::milliseconds time);

    outcome::result<CID> sendBatch(const MapPairStorage &pair_storage_for_send);

    outcome::result<TokenAmount> getSectorCollateral(
        const SectorNumber &sector_number, const TipsetKey &tip_set_key);
  };

}  // namespace fc::mining