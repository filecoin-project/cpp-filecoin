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

namespace fc::mining {
  using api::FullNodeApi;
  using libp2p::protocol::Scheduler;
  using libp2p::protocol::scheduler::Handle;
  using libp2p::protocol::scheduler::Ticks;
  using primitives::SectorNumber;
  using primitives::address::Address;

  class PreCommitBatcherImpl : public PreCommitBatcher {
   public:
    struct PreCommitEntry {
      PreCommitEntry() = default;

      PreCommitEntry(const TokenAmount &number,
                     const SectorPreCommitInfo &info);

      PreCommitEntry &operator=(const PreCommitEntry &other) = default;

      TokenAmount deposit{};
      SectorPreCommitInfo precommit_info;
    };

    PreCommitBatcherImpl(const Ticks &max_time,
                         std::shared_ptr<FullNodeApi> api,
                         const Address &miner_address,
                         const std::shared_ptr<Scheduler> &scheduler);

    void setPreCommitCutoff(const ChainEpoch &current_epoch,
                            const SectorInfo &sector_info);

    outcome::result<void> addPreCommit(
        const SectorInfo &sector_info,
        const TokenAmount &deposit,
        const SectorPreCommitInfo &precommit_info,
        const PrecommitCallback &callback) override;

    void forceSend() override;

    outcome::result<CID> sendBatch();

   private:
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
  };

}  // namespace fc::mining
