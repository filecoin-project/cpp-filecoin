/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <boost/multiprecision/cpp_int.hpp>
#include <chrono>
#include <libp2p/protocol/common/asio/asio_scheduler.hpp>
#include <libp2p/protocol/common/scheduler.hpp>
#include <map>
#include <vm/actor/actor.hpp>
#include "api/full_node/node_api.hpp"
#include "const.hpp"
#include "fsm/fsm.hpp"
#include "miner/storage_fsm/precommit_batcher.hpp"
#include "miner/storage_fsm/sealing_events.hpp"
#include "miner/storage_fsm/types.hpp"
#include "primitives/address/address.hpp"
#include "primitives/types.hpp"
#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/types/miner/sector_info.hpp"
#include "vm/actor/builtin/v0/miner/miner_actor.hpp"

namespace fc::mining {
  using api::FullNodeApi;
  using api::kPushNoSpec;
  using api::SectorNumber;
  using api::SectorPreCommitInfo;
  using boost::multiprecision::cpp_int;
  using fc::vm::actor::builtin::v0::miner::PreCommitBatch;
  using libp2p::protocol::Scheduler;
  using libp2p::protocol::scheduler::Handle;
  using libp2p::protocol::scheduler::Ticks;
  using libp2p::protocol::scheduler::toTicks;
  using primitives::ChainEpoch;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using vm::actor::MethodParams;
  using vm::actor::builtin::types::miner::kChainFinality;
  using StorageFSM =
      fsm::FSM<SealingEvent, SealingEventContext, SealingState, SectorInfo>;

  class PreCommitBatcherImpl : public PreCommitBatcher {
   public:
    struct PreCommitEntry {
      PreCommitEntry(primitives::TokenAmount number, SectorPreCommitInfo info);
      PreCommitEntry() = default;
      primitives::TokenAmount deposit;
      SectorPreCommitInfo precommit_info;
      PreCommitEntry &operator=(const PreCommitEntry & other) = default;
    };

    outcome::result<void> setPreCommitCutoff(primitives::ChainEpoch curEpoch,
                                             const SectorInfo &si);

    static outcome::result<std::shared_ptr<PreCommitBatcherImpl>> makeBatcher(
        size_t maxWait,
        std::shared_ptr<FullNodeApi> api,
        std::shared_ptr<Scheduler> scheduler,
        Address &miner_address);

    outcome::result<void> addPreCommit(SectorInfo secInf,
                                       TokenAmount deposit,
                                       SectorPreCommitInfo pcInfo) override;

    outcome::result<void> forceSend() override;

    outcome::result<void> sendBatch();

   private:
    PreCommitBatcherImpl(size_t maxTime, std::shared_ptr<FullNodeApi> api);
    std::mutex mutex_;
    Address miner_address_;
    cpp_int mutualDeposit;
    std::map<SectorNumber, PreCommitEntry> batchStorage;
    std::shared_ptr<FullNodeApi> api_;
    Handle handle_;
    size_t maxDelay;
    Ticks closestCutoff;
    std::chrono::system_clock::time_point cutoffStart;
  };

}  // namespace fc::mining
