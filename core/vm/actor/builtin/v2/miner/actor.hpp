/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v0/miner/types.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  using v0::miner::ChainEpoch;
  using v0::miner::EpochDuration;
  using v0::miner::RleBitset;
  using v0::miner::SectorPreCommitOnChainInfo;
  using v0::miner::TokenAmount;
  using v0::miner::UvarintKeyer;
  using v0::miner::VestingFunds;

  constexpr EpochDuration kMaxProveCommitDuration{
      v0::miner::kEpochsInDay + v0::miner::kPreCommitChallengeDelay};

  // TODO(turuslan): add miner actor v2 types
  using TodoCid = CID;

  struct State {
    TodoCid info;
    TokenAmount precommit_deposit;
    TokenAmount locked_funds;
    CIDT<VestingFunds> vesting_funds;
    TokenAmount fee_debt;
    TokenAmount initial_pledge;
    adt::Map<SectorPreCommitOnChainInfo, UvarintKeyer> precommitted_sectors;
    adt::Array<RleBitset> precommitted_expiry;
    CIDT<RleBitset> allocated_sectors;
    TodoCid sectors;
    ChainEpoch proving_period_start{};
    uint64_t current_deadline{};
    TodoCid deadlines;
    RleBitset early_terminations;
  };
  CBOR_TUPLE(State,
             info,
             precommit_deposit,
             locked_funds,
             vesting_funds,
             fee_debt,
             initial_pledge,
             precommitted_sectors,
             precommitted_expiry,
             allocated_sectors,
             sectors,
             proving_period_start,
             current_deadline,
             deadlines,
             early_terminations)
}  // namespace fc::vm::actor::builtin::v2::miner

namespace fc {
  template <>
  struct Ipld::Visit<vm::actor::builtin::v2::miner::State> {
    template <typename Visitor>
    static void call(vm::actor::builtin::v2::miner::State &state,
                     const Visitor &visit) {
      visit(state.info);
      visit(state.vesting_funds);
      visit(state.precommitted_sectors);
      visit(state.precommitted_expiry);
      visit(state.allocated_sectors);
      visit(state.sectors);
      visit(state.deadlines);
    }
  };
}  // namespace fc
