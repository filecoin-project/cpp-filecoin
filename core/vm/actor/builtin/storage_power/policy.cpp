/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/storage_power/policy.hpp"

#include <boost/multiprecision/cpp_int.hpp>

namespace fc::vm::actor::builtin::storage_power {

  TokenAmount pledgePenaltyForSectorTermination(
      TokenAmount pledge, SectorTerminationType termination_type) {
    return TokenAmount{0};
  }

  StoragePower consensusPowerForWeight(const SectorStorageWeightDesc &weight) {
    return StoragePower{weight.sector_size};
  }

  StoragePower consensusPowerForWeights(
      gsl::span<const SectorStorageWeightDesc> weights) {
    StoragePower power{0};
    for (const auto weight : weights) {
      power += consensusPowerForWeight(weight);
    }
    return power;
  }

  TokenAmount pledgeForWeight(const SectorStorageWeightDesc &weight,
                              StoragePower network_power) {
    return weight.sector_size * weight.duration * kEpochTotalExpectedReward
           * kPledgeFactor / network_power;
  }

  TokenAmount pledgePenaltyForWindowedPoStFailure(TokenAmount pledge,
                                                  int64_t failures) {
    return TokenAmount{0};
  }

  outcome::result<TokenAmount> pledgePenaltyForConsensusFault(
      TokenAmount pledge, ConsensusFaultType fault_type) {
    switch (fault_type) {
      case ConsensusFaultType::ConsensusFaultDoubleForkMining:
      case ConsensusFaultType::ConsensusFaultParentGrinding:
      case ConsensusFaultType::ConsensusFaultTimeOffsetMining:
        return outcome::success(pledge);
    }
    return outcome::failure(VMExitCode::STORAGE_POWER_ILLEGAL_ARGUMENT);
  }

  TokenAmount rewardForConsensusSlashReport(ChainEpoch elapsed_epoch,
                                            TokenAmount collateral) {
    BigInt slasher_share_numerator = boost::multiprecision::pow(
        ConsensusFaultReporterShareGrowthRate::numerator, elapsed_epoch);
    BigInt slasher_share_denominator = boost::multiprecision::pow(
        ConsensusFaultReporterShareGrowthRate::denominator, elapsed_epoch);

    BigInt num = slasher_share_numerator
                 * ConsensusFaultReporterInitialShare::numerator * collateral;
    BigInt denom = slasher_share_denominator
                   * ConsensusFaultReporterInitialShare::denominator;
    return boost::multiprecision::min((num / denom), collateral);
  }

}  // namespace fc::vm::actor::builtin::storage_power
