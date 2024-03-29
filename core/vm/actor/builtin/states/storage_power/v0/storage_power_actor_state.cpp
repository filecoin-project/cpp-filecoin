/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/states/storage_power/v0/storage_power_actor_state.hpp"

#include "storage/ipfs/datastore.hpp"
#include "vm/actor/builtin/types/storage_power/policy.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::v0::storage_power {
  using types::storage_power::kConsensusMinerMinPower;

  outcome::result<void> PowerActorState::deleteClaim(const Runtime &runtime,
                                                     const Address &address) {
    return claims.remove(address);
  }

  std::tuple<bool, bool> PowerActorState::claimsAreBelow(
      const Claim &old_claim, const Claim &new_claim) const {
    const auto prev_below{old_claim.qa_power < kConsensusMinerMinPower};
    const auto still_below{new_claim.qa_power < kConsensusMinerMinPower};

    return std::make_tuple(prev_below, still_below);
  }

  outcome::result<void> PowerActorState::check(bool condition) const {
    return vm_assert(condition);
  }

}  // namespace fc::vm::actor::builtin::v0::storage_power
