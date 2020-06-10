/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_VM_ACTOR_BUILTIN_STORAGE_POWER_POLICY_HPP
#define CPP_FILECOIN_VM_ACTOR_BUILTIN_STORAGE_POWER_POLICY_HPP

#include "vm/actor/builtin/reward/reward_actor.hpp"

namespace fc::vm::actor::builtin::storage_power {
  using fc::primitives::EpochDuration;
  using fc::primitives::SectorStorageWeightDesc;
  using fc::primitives::StoragePower;
  using fc::primitives::TokenAmount;

  static const StoragePower kConsensusMinerMinPower{StoragePower{1} << 40};
  constexpr size_t kSectorQualityPrecision{20};

  enum class SectorTerminationType {
    EXPIRED,
    MANUAL,
    FAULTY,
  };

  StoragePower qaPowerForWeight(const SectorStorageWeightDesc &weight);

  inline TokenAmount initialPledgeForWeight(
      const StoragePower &qa,
      const StoragePower &total_qa,
      const TokenAmount &circ_supply,
      const TokenAmount &total_pledge,
      const TokenAmount &per_epoch_reward) {
    return (qa * per_epoch_reward) / total_qa;
  }
}  // namespace fc::vm::actor::builtin::storage_power

#endif  // CPP_FILECOIN_VM_ACTOR_BUILTIN_STORAGE_POWER_POLICY_HPP
