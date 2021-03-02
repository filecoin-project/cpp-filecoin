/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/storage_power/storage_power_actor_utils.hpp"

namespace fc::vm::actor::builtin::v2::storage_power {
  outcome::result<void> PowerUtils::validateMinerHasClaim(
      states::PowerActorStatePtr state, const Address &miner) const {
    REQUIRE_NO_ERROR(state->loadClaimsRoot(), VMExitCode::kErrIllegalState);
    REQUIRE_NO_ERROR_A(
        has, state->hasClaim(miner), VMExitCode::kErrIllegalState);
    if (!has) {
      ABORT(VMExitCode::kErrForbidden);
    }
    return outcome::success();
  }
}  // namespace fc::vm::actor::builtin::v2::storage_power
