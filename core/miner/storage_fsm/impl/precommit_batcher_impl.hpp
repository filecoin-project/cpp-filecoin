/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "miner/storage_fsm/precommit_batcher.hpp"

#include <chrono>
#include <libp2p/protocol/common/scheduler.hpp>
#include <map>
#include <mutex>
#include "api/full_node/node_api.hpp"
#include "primitives/address/address.hpp"
#include "miner/storage_fsm/types.hpp"

namespace fc::mining {
  using api::FullNodeApi;
  using libp2p::protocol::Scheduler;
  using libp2p::protocol::scheduler::Handle;
  using libp2p::protocol::scheduler::Ticks;
  using primitives::SectorNumber;
  using primitives::address::Address;
  using types::FeeConfig;

  class PreCommitBatcherImpl : public PreCommitBatcher {
   public:
    PreCommitBatcherImpl(const Ticks &max_time,
                         std::shared_ptr<FullNodeApi> api,
                         const Address &miner_address,
                         const std::shared_ptr<Scheduler> &scheduler,
                         const std::function<outcome::result<Address>(MinerInfo miner_info, TokenAmount deposit, TokenAmount good_funds)>&,
                          std::shared_ptr<FeeConfig> fee_config);

    outcome::result<void> addPreCommit(
        const SectorInfo &sector_info,
        const TokenAmount &deposit,
        const SectorPreCommitInfo &precommit_info,
        const PrecommitCallback &callback) override;

    void forceSend() override;

   private:
    struct PreCommitEntry {
      PreCommitEntry() = default;

      PreCommitEntry(const TokenAmount &number,
                     const SectorPreCommitInfo &info);

      PreCommitEntry &operator=(const PreCommitEntry &other) = default;

      TokenAmount deposit{};
      SectorPreCommitInfo precommit_info;
    };

    std::mutex mutex_;
    TokenAmount mutual_deposit_;
    std::map<SectorNumber, PreCommitEntry> batch_storage_;
    Ticks max_delay_;
    std::shared_ptr<FullNodeApi> api_;
    Address miner_address_;
    Handle handle_;
    Ticks closest_cutoff_;
    std::chrono::system_clock::time_point cutoff_start_;
    common::Logger logger_;
    std::map<SectorNumber, PrecommitCallback> callbacks_;
    std::shared_ptr<FeeConfig> fee_config_;
    std::function<outcome::result<Address>(MinerInfo miner_info, TokenAmount deposit, TokenAmount good_funds)> address_selector_;

    void forceSendWithoutLock();

    void setPreCommitCutoff(const ChainEpoch &current_epoch,
                            const SectorInfo &sector_info);

    outcome::result<CID> sendBatch();
  };

}  // namespace fc::mining
