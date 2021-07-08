/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <boost/multiprecision/cpp_int.hpp>
#include <libp2p/protocol/common/asio/asio_scheduler.hpp>
#include <libp2p/protocol/common/scheduler.hpp>
#include <map>
#include <vm/actor/actor.hpp>
#include <chrono>
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
  using api::SectorNumber;
  using api::SectorPreCommitInfo;
  using api::kPushNoSpec;

  using boost::multiprecision::cpp_int;
  using fc::mining::types::SectorInfo;
  using libp2p::protocol::scheduler::Handle;
  using libp2p::protocol::scheduler::toTicks;
  using primitives::address::Address;
  using vm::actor::MethodParams;
  using StorageFSM =
      fsm::FSM<SealingEvent, SealingEventContext, SealingState, SectorInfo>;

  class PreCommitBatcherImpl : public PreCommitBatcher {
   public:
    struct preCommitEntry {
      preCommitEntry(primitives::TokenAmount number,
                     SectorPreCommitInfo info);
      preCommitEntry() = default;
      primitives::TokenAmount deposit;
      SectorPreCommitInfo pci;
      preCommitEntry& operator=(const preCommitEntry &x);
    };

    void getPreCommitCutoff(primitives::ChainEpoch curEpoch,
                                             const SectorInfo & si);

    static outcome::result<std::shared_ptr<PreCommitBatcherImpl>> makeBatcher(size_t maxWait, std::shared_ptr<FullNodeApi>  api,
                                                                               std::shared_ptr<libp2p::protocol::Scheduler> scheduler, Address & miner_address);

    outcome::result<void> addPreCommit(SectorInfo secInf,
                                       primitives::TokenAmount deposit,
                                       SectorPreCommitInfo pcInfo);

    outcome::result<void> forceSend();

    outcome::result<void> sendBatch();


   private:
    PreCommitBatcherImpl(size_t maxTime,  std::shared_ptr<FullNodeApi> api, std::shared_ptr<libp2p::protocol::Scheduler> scheduler);
    std::mutex mutex_;
    Address miner_address_;
    cpp_int mutualDeposit;
    std::map<uint64_t, preCommitEntry> batchStorage;
    std::shared_ptr<FullNodeApi> api_;
    std::shared_ptr<libp2p::protocol::Scheduler> sch_;
    Handle handle_;
    size_t maxDelay;
  };

}  // namespace fc::mining
