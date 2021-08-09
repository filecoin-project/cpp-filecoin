/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/states/market/market_actor_state.hpp"
#include "vm/actor/builtin/types/market/policy.hpp"
#include "vm/exit_code/exit_code.hpp"

namespace fc::vm::actor::builtin::v5::market {
  using primitives::DealId;
  using primitives::SpaceTime;
  using primitives::address::Address;
  using primitives::piece::PaddedPieceSize;

  struct validate_RESULT {
    PaddedPieceSize space;
    SpaceTime space_time;
    SpaceTime space_time_verified;
  };

  inline outcome::result<validate_RESULT> validate(
      const states::MarketActorStatePtr &state,
      const Address &miner,
      const std::vector<DealId> &deals,
      ChainEpoch activation,
      ChainEpoch expiration) {
    // TODO(turuslan): demangle specs-actors error flow
    constexpr auto kTodoError{VMExitCode::kNotImplemented};
    std::set<DealId> seen;
    validate_RESULT result;
    for (const auto &id : deals) {
      if (!seen.insert(id).second) {
        return kTodoError;
      }
      OUTCOME_TRY(proposal, state->proposals.get(id));
      if (proposal.provider != miner) {
        return kTodoError;
      }
      if (activation > proposal.start_epoch) {
        return kTodoError;
      }
      if (proposal.end_epoch > expiration) {
        return kTodoError;
      }
      result.space += proposal.piece_size;
      (proposal.verified ? result.space_time_verified : result.space_time) +=
          types::market::dealWeight(proposal);
    }
    return result;
  }
}  // namespace fc::vm::actor::builtin::v5::market
