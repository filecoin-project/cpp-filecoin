/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/states/miner/v0/miner_actor_state.hpp"

#include "codec/cbor/streams_annotation.hpp"
#include "storage/ipfs/datastore.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  using primitives::ChainEpoch;
  using primitives::SectorSize;
  using primitives::TokenAmount;
  using types::Universal;
  using types::miner::DeadlineSectorMap;
  using types::miner::MinerInfo;
  using types::miner::PowerPair;
  using types::miner::SectorOnChainInfo;

  struct MinerActorState : v0::miner::MinerActorState {
    outcome::result<Universal<MinerInfo>> getInfo() const override;

    outcome::result<std::vector<SectorOnChainInfo>> rescheduleSectorExpirations(
        ChainEpoch curr_epoch,
        SectorSize ssize,
        const DeadlineSectorMap &deadline_sectors) override;

    outcome::result<TokenAmount> unlockUnvestedFunds(
        ChainEpoch curr_epoch, const TokenAmount &target) override;

    outcome::result<TokenAmount> unlockVestedFunds(
        ChainEpoch curr_epoch) override;

    outcome::result<TokenAmount> getUnlockedBalance(
        const TokenAmount &actor_balance) const override;

    outcome::result<TokenAmount> getAvailableBalance(
        const TokenAmount &actor_balance) const override;

    outcome::result<void> checkBalanceInvariants(
        const TokenAmount &balance) const override;
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

}  // namespace fc::vm::actor::builtin::v2::miner

namespace fc::cbor_blake {
  template <>
  struct CbVisitT<vm::actor::builtin::v2::miner::MinerActorState> {
    template <typename Visitor>
    static void call(vm::actor::builtin::v2::miner::MinerActorState &state,
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
