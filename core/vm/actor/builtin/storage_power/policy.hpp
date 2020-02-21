/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_VM_ACTOR_BUILTIN_STORAGE_POWER_POLICY_HPP
#define CPP_FILECOIN_VM_ACTOR_BUILTIN_STORAGE_POWER_POLICY_HPP

#include "vm/actor/builtin/reward/reward_actor.hpp"
#include "vm/actor/util.hpp"

namespace fc::vm::actor::builtin::storage_power {

  using fc::vm::actor::builtin::reward::kBlockRewardTarget;
  using StoragePower = primitives::BigInt;

  /**
   * Total expected block reward per epoch (per-winner reward * expected
   * winners), as input to pledge requirement
   */
  inline static const TokenAmount kEpochTotalExpectedReward{kBlockRewardTarget
                                                            * 5};

  /**
   * Multiplier on sector pledge requirement
   */
  inline static const BigInt kPledgeFactor{3};

  enum class SectorTerminationType {
    // Implicit termination after all deals expire
    SECTOR_TERMINATION_EXPIRED = 1,

    // Unscheduled explicit termination by the miner
    SECTOR_TERMINATION_MANUAL
  };

  /**
   * Penalty to pledge collateral for the termination of an individual sector
   */
  TokenAmount pledgePenaltyForSectorTermination(
      TokenAmount pledge, SectorTerminationType termination_type);

  StoragePower consensusPowerForWeight(const SectorStorageWeightDescr &weight);

  StoragePower consensusPowerForWeights(
      gsl::span<SectorStorageWeightDescr> weights);

  TokenAmount pledgeForWeight(const SectorStorageWeightDescr &weight,
                              StoragePower network_power);

}  // namespace fc::vm::actor::builtin::storage_power

#endif  // CPP_FILECOIN_VM_ACTOR_BUILTIN_STORAGE_POWER_POLICY_HPP
