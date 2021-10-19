/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/full_node/node_api.hpp"
#include "sector_storage/fault_tracker.hpp"
#include "sector_storage/spec_interfaces/prover.hpp"
#include "vm/actor/builtin/v0/miner/miner_actor.hpp"

namespace fc::mining {
  using api::Address;
  using api::ChainEpoch;
  using api::DeadlineInfo;
  using api::FullNodeApi;
  using api::RleBitset;
  using api::TipsetCPtr;
  using sector_storage::FaultTracker;
  using sector_storage::Prover;
  using sector_storage::RegisteredPoStProof;
  using vm::actor::builtin::v0::miner::SubmitWindowedPoSt;
  using vm::message::MethodNumber;

  // synchronous WindowPoSt
  struct WindowPoStScheduler
      : public std::enable_shared_from_this<WindowPoStScheduler> {
    static constexpr auto kStartConfidence{4};

    struct Cached {
      DeadlineInfo deadline;
      std::vector<SubmitWindowedPoSt::Params> params;
      size_t submitted;
    };

    static outcome::result<std::shared_ptr<WindowPoStScheduler>> create(
        std::shared_ptr<FullNodeApi> api,
        std::shared_ptr<Prover> prover,
        std::shared_ptr<FaultTracker> fault_tracker,
        const Address &miner);
    void onChange(TipsetCPtr revert, TipsetCPtr apply);
    outcome::result<RleBitset> checkSectors(const RleBitset &sectors, bool ok);
    outcome::result<void> pushMessage(MethodNumber method, Bytes params);

    std::shared_ptr<api::Channel<std::vector<api::HeadChange>>> channel;
    std::shared_ptr<FullNodeApi> api;
    std::shared_ptr<Prover> prover;
    std::shared_ptr<FaultTracker> fault_tracker;
    Address miner, worker;
    // TODO(turuslan): FIL-420 check cache memory usage
    std::map<ChainEpoch, Cached> cache;
    uint64_t part_size;
    RegisteredPoStProof proof_type;
  };
}  // namespace fc::mining
