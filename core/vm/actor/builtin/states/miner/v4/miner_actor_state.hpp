/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/states/miner/v3/miner_actor_state.hpp"

#include "codec/cbor/streams_annotation.hpp"
#include "storage/ipfs/datastore.hpp"

namespace fc::vm::actor::builtin::v4::miner {
  using types::Universal;
  using types::miner::MinerInfo;

  struct MinerActorState : v3::miner::MinerActorState {
    outcome::result<Universal<MinerInfo>> getInfo() const override;
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
             early_terminations,
             deadline_cron_active)

}  // namespace fc::vm::actor::builtin::v4::miner

namespace fc::cbor_blake {
  template <>
  struct CbVisitT<vm::actor::builtin::v4::miner::MinerActorState> {
    template <typename Visitor>
    static void call(vm::actor::builtin::v4::miner::MinerActorState &state,
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
