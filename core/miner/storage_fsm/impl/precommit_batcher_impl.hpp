/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "miner/storage_fsm/precommit_batcher.hpp"

#include <chrono>
#include <libp2p/basic/scheduler.hpp>
#include <map>
#include <mutex>
#include "miner/storage_fsm/types.hpp"
#include "primitives/address/address.hpp"

namespace fc::mining {
  using libp2p::basic::Scheduler;
  using primitives::SectorNumber;
  using primitives::address::Address;
  using types::FeeConfig;

  class PreCommitBatcherImpl : public PreCommitBatcher {
   public:
    PreCommitBatcherImpl(const std::chrono::milliseconds &max_time,
                         std::shared_ptr<FullNodeApi> api,
                         const Address &miner_address,
                         const std::shared_ptr<Scheduler> &scheduler,
                         const AddressSelector &address_selector,
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

      PreCommitEntry(const PreCommitEntry &) = delete;
      PreCommitEntry(PreCommitEntry &&) = delete;

      PreCommitEntry(const TokenAmount &number,
                     const SectorPreCommitInfo &info);

      ~PreCommitEntry() = default;

      PreCommitEntry &operator=(const PreCommitEntry &other) noexcept = default;
      PreCommitEntry &operator=(PreCommitEntry &&) noexcept = default;

      TokenAmount deposit{};
      SectorPreCommitInfo precommit_info;
    };

    std::mutex mutex_;
    // TODO(turuslan): FIL-420 check cache memory usage
    std::map<SectorNumber, PreCommitEntry> batch_storage_;
    std::chrono::milliseconds max_delay_;
    std::shared_ptr<FullNodeApi> api_;
    Address miner_address_;
    std::shared_ptr<Scheduler> scheduler_;
    Scheduler::Handle handle_;
    std::chrono::milliseconds closest_cutoff_;
    std::chrono::system_clock::time_point cutoff_start_;
    common::Logger logger_;
    // TODO(turuslan): FIL-420 check cache memory usage
    std::map<SectorNumber, PrecommitCallback> callbacks_;
    std::shared_ptr<FeeConfig> fee_config_;
    AddressSelector address_selector_;
    void forceSendWithoutLock();

    void setPreCommitCutoff(const ChainEpoch &current_epoch,
                            const SectorInfo &sector_info);

    outcome::result<CID> sendBatch();

    void reschedule(std::chrono::milliseconds time);
  };

}  // namespace fc::mining
