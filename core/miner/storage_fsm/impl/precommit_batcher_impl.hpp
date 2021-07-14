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
#include <vm/actor/actor.hpp>
#include "api/full_node/node_api.hpp"
#include "miner/storage_fsm/precommit_batcher.hpp"
#include "miner/storage_fsm/types.hpp"
#include "primitives/address/address.hpp"
#include "primitives/types.hpp"
#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/types/miner/sector_info.hpp"

namespace fc::mining {
  using api::FullNodeApi;
  using api::SectorNumber;
  using api::SectorPreCommitInfo;
  using boost::multiprecision::cpp_int;
  using libp2p::protocol::Scheduler;
  using libp2p::protocol::scheduler::Handle;
  using libp2p::protocol::scheduler::Ticks;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using vm::actor::MethodParams;

  class PreCommitBatcherImpl : public PreCommitBatcher {
   public:
    struct PreCommitEntry {
      PreCommitEntry(primitives::TokenAmount number, SectorPreCommitInfo info);
      PreCommitEntry() = default;
      primitives::TokenAmount deposit;
      SectorPreCommitInfo precommit_info;
      PreCommitEntry &operator=(const PreCommitEntry &other) = default;
    };

    outcome::result<void> setPreCommitCutoff(const ChainEpoch &curEpoch,
                                             const SectorInfo &si);

    static outcome::result<std::shared_ptr<PreCommitBatcherImpl>> makeBatcher(
        const Ticks &maxWait,
        std::shared_ptr<FullNodeApi> api,
        std::shared_ptr<Scheduler> scheduler,
        const Address &miner_address);

    outcome::result<void> addPreCommit(
        const SectorInfo &secInf,
        const TokenAmount &deposit,
        const SectorPreCommitInfo &pcInfo) override;

    outcome::result<void> forceSend() override;

    void sendBatch();
    PreCommitBatcherImpl(const Ticks &maxTime,
                         std::shared_ptr<FullNodeApi> api,
                         const Address &miner_address,
                         const Ticks &closest_cutoff);

   private:
    std::mutex mutex_;
    cpp_int mutual_deposit_;
    std::map<SectorNumber, PreCommitEntry> batch_storage_;
    Ticks max_delay_;
    std::shared_ptr<FullNodeApi> api_;
    Address miner_address_;
    Handle handle_;
    Ticks closest_cutoff_;
    std::chrono::system_clock::time_point cutoff_start_;
    common::Logger logger_;
  };

}  // namespace fc::mining
