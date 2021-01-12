/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/api.hpp"
#include "sector_storage/fault_tracker.hpp"
#include "sector_storage/spec_interfaces/prover.hpp"
#include "vm/actor/builtin/v0/miner/miner_actor.hpp"

namespace fc::mining {
  using api::Address;
  using api::Api;
  using api::ChainEpoch;
  using api::DeadlineInfo;
  using api::RleBitset;
  using api::TipsetCPtr;
  using sector_storage::FaultTracker;
  using sector_storage::Prover;
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
        std::shared_ptr<Api> api,
        std::shared_ptr<Prover> prover,
        std::shared_ptr<FaultTracker> fault_tracker,
        const Address &miner);
    void onChange(TipsetCPtr revert, TipsetCPtr apply);
    outcome::result<RleBitset> checkSectors(const RleBitset &sectors, bool ok);
    outcome::result<void> pushMessage(MethodNumber method, Buffer params);

    std::shared_ptr<api::Channel<std::vector<api::HeadChange>>> channel;
    std::shared_ptr<Api> api;
    std::shared_ptr<Prover> prover;
    std::shared_ptr<FaultTracker> fault_tracker;
    Address miner, worker;
    std::map<ChainEpoch, Cached> cache;
    uint64_t part_size;
    RegisteredProof proof_type;
  };
}  // namespace fc::mining
