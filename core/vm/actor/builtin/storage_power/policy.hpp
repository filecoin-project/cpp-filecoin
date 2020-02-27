/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_VM_ACTOR_BUILTIN_STORAGE_POWER_POLICY_HPP
#define CPP_FILECOIN_VM_ACTOR_BUILTIN_STORAGE_POWER_POLICY_HPP

#include "vm/actor/builtin/reward/reward_actor.hpp"

namespace fc::vm::actor::builtin::storage_power {

  using fc::primitives::SectorStorageWeightDesc;
  using fc::primitives::StoragePower;
  using fc::primitives::TokenAmount;
  using fc::vm::actor::builtin::reward::kBlockRewardTarget;

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

  enum class SectorTerminationType : int {
    // Implicit termination after all deals expire
    SECTOR_TERMINATION_EXPIRED = 0,

    // Unscheduled explicit termination by the miner
    SECTOR_TERMINATION_MANUAL
  };

  /**
   * Penalty to pledge collateral for the termination of an individual sector
   */
  TokenAmount pledgePenaltyForSectorTermination(
      TokenAmount pledge, SectorTerminationType termination_type);

  StoragePower consensusPowerForWeight(const SectorStorageWeightDesc &weight);

  StoragePower consensusPowerForWeights(
      gsl::span<const SectorStorageWeightDesc> weights);

  TokenAmount pledgeForWeight(const SectorStorageWeightDesc &weight,
                              StoragePower network_power);

}  // namespace fc::vm::actor::builtin::storage_power

#endif  // CPP_FILECOIN_VM_ACTOR_BUILTIN_STORAGE_POWER_POLICY_HPP
