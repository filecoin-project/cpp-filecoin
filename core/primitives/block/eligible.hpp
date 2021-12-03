/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cbor_blake/ipld_version.hpp"
#include "primitives/tipset/tipset.hpp"
#include "vm/actor/builtin/states/miner/miner_actor_state.hpp"
#include "vm/actor/builtin/states/storage_power/storage_power_actor_state.hpp"
#include "vm/actor/builtin/types/storage_power/policy.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

namespace fc::miner_eligible_to_miner {
  using primitives::address::Address;
  using primitives::tipset::TipsetCPtr;
  using vm::actor::kStoragePowerAddress;
  using vm::actor::builtin::states::Claim;
  using vm::actor::builtin::states::MinerActorStatePtr;
  using vm::actor::builtin::states::PowerActorStatePtr;
  using vm::state::StateTreeImpl;

  inline bool minerHasMinPower(const PowerActorStatePtr &state,
                               const Claim &claim) {
    const auto &power{state.actor_version == ActorVersion::kVersion0
                          ? claim.qa_power
                          : claim.raw_power};
    return state->num_miners_meeting_min_power < kConsensusMinerMinMiners
               ? !power.is_zero()
               : power > vm::actor::builtin::types::storage_power::
                         kConsensusMinerMinPower;
  }

  inline outcome::result<bool> minerEligibleToMine(const Address &miner,
                                                   const TipsetCPtr &lookback,
                                                   const TipsetCPtr &parent,
                                                   StateTreeImpl &parent_tree) {
    StateTreeImpl lookback_tree{
        withVersion(parent_tree.getStore(), lookback->height()),
        lookback->getParentStateRoot()};
    OUTCOME_TRY(lookback_power_actor, lookback_tree.get(kStoragePowerAddress));
    OUTCOME_TRY(lookback_power,
                getCbor<PowerActorStatePtr>(lookback_tree.getStore(),
                                            lookback_power_actor.head));
    OUTCOME_TRY(lookback_claim, lookback_power->getClaim(miner));
    if (!minerHasMinPower(lookback_power, *lookback_claim)) {
      return false;
    }
    if (vm::version::getNetworkVersion(parent->height())
        <= vm::version::NetworkVersion::kVersion3) {
      return true;
    }
    OUTCOME_TRY(parent_power_actor, parent_tree.get(kStoragePowerAddress));
    OUTCOME_TRY(parent_power,
                getCbor<PowerActorStatePtr>(parent_tree.getStore(),
                                            parent_power_actor.head));
    OUTCOME_TRY(parent_claim, parent_power->tryGetClaim(miner));
    if (!parent_claim || (**parent_claim).qa_power <= 0) {
      return false;
    }
    OUTCOME_TRY(parent_miner_actor, parent_tree.get(miner));
    OUTCOME_TRY(parent_miner,
                getCbor<MinerActorStatePtr>(parent_tree.getStore(),
                                            parent_miner_actor.head));
    if (!parent_miner->fee_debt.is_zero()) {
      return false;
    }
    OUTCOME_TRY(parent_miner_info, parent_miner->getInfo());
    if (parent->height() <= parent_miner_info->consensus_fault_elapsed) {
      return false;
    }
    return true;
  }
}  // namespace fc::miner_eligible_to_miner

namespace fc {
  using miner_eligible_to_miner::minerEligibleToMine;
}  // namespace fc
