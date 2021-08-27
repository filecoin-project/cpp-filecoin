/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/states/miner/v2/miner_actor_state.hpp"

#include "codec/cbor/streams_annotation.hpp"
#include "storage/ipfs/datastore.hpp"

namespace fc::vm::actor::builtin::v3::miner {
  using primitives::ChainEpoch;
  using primitives::SectorSize;
  using runtime::Runtime;
  using types::Universal;
  using types::miner::MinerInfo;
  using types::miner::PowerPair;
  using types::miner::SectorOnChainInfo;

  struct MinerActorState : v2::miner::MinerActorState {
    outcome::result<Universal<MinerInfo>> getInfo() const override;

    outcome::result<PowerPair> assignSectorsToDeadlines(
        Runtime &runtime,
        ChainEpoch curr_epoch,
        const std::vector<SectorOnChainInfo> &sectors_to_assign,
        uint64_t partition_size,
        SectorSize ssize) override;
  };
  CBOR_TUPLE(MinerActorState,
             miner_info,
             precommit_deposit,
             locked_funds,
             vesting_funds,
             fee_debt,
             initial_pledge,
             precommitted_sectors,
             precommitted_setctors_expiry,
             allocated_sectors,
             sectors,
             proving_period_start,
             current_deadline,
             deadlines,
             early_terminations)

}  // namespace fc::vm::actor::builtin::v3::miner

namespace fc::cbor_blake {
  template <>
  struct CbVisitT<vm::actor::builtin::v3::miner::MinerActorState> {
    template <typename Visitor>
    static void call(vm::actor::builtin::v3::miner::MinerActorState &state,
                     const Visitor &visit) {
      visit(state.miner_info);
      visit(state.vesting_funds);
      visit(state.precommitted_sectors);
      visit(state.precommitted_setctors_expiry);
      visit(state.allocated_sectors);
      visit(state.sectors);
      visit(state.deadlines);
    }
  };
}  // namespace fc::cbor_blake
