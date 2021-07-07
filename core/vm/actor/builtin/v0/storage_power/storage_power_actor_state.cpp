/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/storage_power/storage_power_actor_state.hpp"

#include "storage/ipfs/datastore.hpp"
#include "vm/actor/builtin/types/storage_power/policy.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::v0::storage_power {
  using types::Universal;
  using types::storage_power::Claim;
  using types::storage_power::kConsensusMinerMinPower;

  ACTOR_STATE_TO_CBOR_THIS(PowerActorState)

  std::shared_ptr<states::PowerActorState> PowerActorState::copy() const {
    auto copy = std::make_shared<PowerActorState>(*this);
    return copy;
  }

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

}  // namespace fc::vm::actor::builtin::v0::storage_power
