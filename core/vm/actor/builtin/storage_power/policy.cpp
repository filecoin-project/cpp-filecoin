/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/storage_power/policy.hpp"

namespace fc::vm::actor::builtin::storage_power {
  using primitives::SectorQuality;

  SectorQuality sectorQualityFromWeight(const SectorStorageWeightDesc &weight) {
    constexpr auto kBaseMultiplier{10};
    constexpr auto kDealWeightMultiplier{11};
    constexpr auto kVerifiedDealWeightMultiplier{100};

    BigInt sector{BigInt{weight.sector_size} * weight.duration};
    BigInt total_deal{weight.deal_weight + weight.verified_deal_weight};
    assert(sector > 0);
    return bigdiv(
        bigdiv((kBaseMultiplier * (sector - total_deal)
                + weight.deal_weight * kDealWeightMultiplier
                + weight.verified_deal_weight * kVerifiedDealWeightMultiplier)
                   << kSectorQualityPrecision,
               sector),
        kBaseMultiplier);
  }

  StoragePower qaPowerForWeight(const SectorStorageWeightDesc &weight) {
    return (weight.sector_size * sectorQualityFromWeight(weight))
           >> kSectorQualityPrecision;
  }
}  // namespace fc::vm::actor::builtin::storage_power
