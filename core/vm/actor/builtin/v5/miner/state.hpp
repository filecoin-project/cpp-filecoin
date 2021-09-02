/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/states/miner/miner_actor_state.hpp"

// TODO(m.tagirov): refactor
namespace fc::vm::actor::builtin::v5::miner {
  using states::MinerActorStatePtr;

  inline outcome::result<TokenAmount> checkVestedFunds(
      const MinerActorStatePtr &state, ChainEpoch now) {
    OUTCOME_TRY(vesting, state->vesting_funds.get());
    TokenAmount sum;
    for (auto &fund : vesting.funds) {
      if (fund.epoch >= now) {
        break;
      }
      sum += fund.amount;
    }
    return sum;
  }

  inline std::optional<TokenAmount> getUnlockedBalance(
      const MinerActorStatePtr &state, const TokenAmount &actor) {
    TokenAmount unlocked{actor - state->locked_funds - state->precommit_deposit
                         - state->initial_pledge};
    if (unlocked < 0) {
      return {};
    }
    return unlocked;
  }

  inline std::optional<TokenAmount> getAvailableBalance(
      const MinerActorStatePtr &state, const TokenAmount &actor) {
    if (auto unlocked{getUnlockedBalance(state, actor)}) {
      return *unlocked - state->fee_debt;
    }
    return {};
  }
}  // namespace fc::vm::actor::builtin::v5::miner
