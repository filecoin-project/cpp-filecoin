/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/storage_power/policy.hpp"

namespace fc::vm::actor::builtin::storage_power {

  TokenAmount pledgePenaltyForSectorTermination(
      TokenAmount pledge, SectorTerminationType termination_type) {
    return TokenAmount{0};
  }

  StoragePower consensusPowerForWeight(const SectorStorageWeightDescr &weight) {
    return StoragePower{weight.sector_size};
  }

  StoragePower consensusPowerForWeights(
      gsl::span<SectorStorageWeightDescr> weights) {
    StoragePower power{0};
    for (auto weight : weights) {
      power += consensusPowerForWeight(weight);
    }
    return power;
  }

  TokenAmount pledgeForWeight(const SectorStorageWeightDescr &weight,
                              StoragePower network_power) {
    return weight.sector_size * weight.duration * kEpochTotalExpectedReward
           * kPledgeFactor / network_power;
  }
}  // namespace fc::vm::actor::builtin::storage_power
