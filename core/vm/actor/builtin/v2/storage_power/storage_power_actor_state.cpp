/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/storage_power/storage_power_actor_state.hpp"

#include "storage/ipfs/datastore.hpp"
#include "vm/actor/builtin/types/storage_power/policy.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::v2::storage_power {
  using types::storage_power::kConsensusMinerMinPower;

  outcome::result<void> PowerActorState::deleteClaim(const Runtime &runtime,
                                                     const Address &address) {
    OUTCOME_TRY(claim_found, tryGetClaim(address));
    if (!claim_found.has_value()) {
      return outcome::success();
    }
    // subtract from stats as if we were simply removing power
    OUTCOME_TRY(addToClaim(runtime,
                           address,
                           -claim_found.value()->raw_power,
                           -claim_found.value()->qa_power));
    // delete claim from state to invalidate miner
    OUTCOME_TRY(claims.remove(address));
    return outcome::success();
  }

  std::tuple<bool, bool> PowerActorState::claimsAreBelow(
      const Claim &old_claim, const Claim &new_claim) const {
    const auto prev_below{old_claim.raw_power < kConsensusMinerMinPower};
    const auto still_below{new_claim.raw_power < kConsensusMinerMinPower};

    return std::make_tuple(prev_below, still_below);
  }
}  // namespace fc::vm::actor::builtin::v2::storage_power
