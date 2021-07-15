/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/multiprecision/cpp_int.hpp>
#include <chrono>
#include <libp2p/protocol/common/asio/asio_scheduler.hpp>
#include <libp2p/protocol/common/scheduler.hpp>
#include <map>
#include "api/full_node/node_api.hpp"
#include "miner/storage_fsm/precommit_batcher.hpp"
#include "miner/storage_fsm/types.hpp"
#include "primitives/address/address.hpp"
#include "primitives/types.hpp"
#include "vm/actor/actor.hpp"

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
      PreCommitEntry(const TokenAmount &number,
                     const SectorPreCommitInfo &info);
      PreCommitEntry() = default;
      TokenAmount deposit;
      SectorPreCommitInfo precommit_info;
      PreCommitEntry &operator=(const PreCommitEntry &other) = default;
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
        const PrecommitCallback &callback);

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
