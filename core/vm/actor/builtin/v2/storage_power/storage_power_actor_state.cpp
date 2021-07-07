/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/storage_power/storage_power_actor_state.hpp"

#include "storage/ipfs/datastore.hpp"
#include "vm/actor/builtin/types/storage_power/policy.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::v2::storage_power {
  using runtime::Runtime;
  using types::storage_power::kConsensusMinerMinPower;

  ACTOR_STATE_TO_CBOR_THIS(PowerActorState)

  std::shared_ptr<states::PowerActorState> PowerActorState::copy() const {
    auto copy = std::make_shared<PowerActorState>(*this);
    return copy;
  }

  outcome::result<void> PowerActorState::setClaim(
      const Runtime &runtime,
      const Address &address,
      const StoragePower &raw,
      const StoragePower &qa,
      RegisteredSealProof seal_proof) {
    VM_ASSERT(raw >= 0);
    VM_ASSERT(qa >= 0);

    states::storage_power::ClaimV2 claim;
    claim.seal_proof_type = seal_proof;
    claim.raw_power = raw;
    claim.qa_power = qa;

    OUTCOME_TRY(claims2.set(address, claim));
    return outcome::success();
  }

  outcome::result<void> PowerActorState::deleteClaim(const Runtime &runtime,
                                                     const Address &address) {
    OUTCOME_TRY(claim_found, tryGetClaim(address));
    if (!claim_found.has_value()) {
      return outcome::success();
    }
    // subtract from stats as if we were simply removing power
    OUTCOME_TRY(addToClaim(runtime,
                           address,
                           -claim_found.value().raw_power,
                           -claim_found.value().qa_power));
    // delete claim from state to invalidate miner
    OUTCOME_TRY(claims2.remove(address));
    return outcome::success();
  }

  outcome::result<bool> PowerActorState::hasClaim(
      const Address &address) const {
    return claims2.has(address);
  }

  outcome::result<boost::optional<Claim>> PowerActorState::tryGetClaim(
      const Address &address) const {
    OUTCOME_TRY(claim, claims2.tryGet(address));
    if (claim) {
      return *claim;
    }
    return boost::none;
  }

  outcome::result<Claim> PowerActorState::getClaim(
      const Address &address) const {
    OUTCOME_TRY(claim, claims2.get(address));
    return std::move(claim);
  }

  outcome::result<std::vector<adt::AddressKeyer::Key>>
  PowerActorState::getClaimsKeys() const {
    return claims2.keys();
  }

  outcome::result<void> PowerActorState::loadClaimsRoot() {
    return claims2.hamt.loadRoot();
  }

  std::tuple<bool, bool> PowerActorState::claimsAreBelow(
      const Claim &old_claim, const Claim &new_claim) const {
    const auto prev_below{old_claim.raw_power < kConsensusMinerMinPower};
    const auto still_below{new_claim.raw_power < kConsensusMinerMinPower};

    return std::make_tuple(prev_below, still_below);
  }
}  // namespace fc::vm::actor::builtin::v2::storage_power
