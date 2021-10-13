/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/v3/proof_policy.hpp"

#include "const.hpp"

namespace fc::vm::actor::builtin::v3::miner {
  using primitives::sector::Errors;

  outcome::result<ChainEpoch> ProofPolicy::getSealProofSectorMaximumLifetime(
      RegisteredSealProof proof, NetworkVersion nv) const {
    const auto epochs_in_five_years = 5 * kEpochsInYear;
    const auto epochs_in_540_days = 540 * kEpochsInDay;

    if (nv < NetworkVersion::kVersion11) {
      switch (proof) {
        case RegisteredSealProof::kStackedDrg2KiBV1:
        case RegisteredSealProof::kStackedDrg8MiBV1:
        case RegisteredSealProof::kStackedDrg512MiBV1:
        case RegisteredSealProof::kStackedDrg32GiBV1:
        case RegisteredSealProof::kStackedDrg64GiBV1:
        case RegisteredSealProof::kStackedDrg2KiBV1_1:
        case RegisteredSealProof::kStackedDrg8MiBV1_1:
        case RegisteredSealProof::kStackedDrg512MiBV1_1:
        case RegisteredSealProof::kStackedDrg32GiBV1_1:
        case RegisteredSealProof::kStackedDrg64GiBV1_1:
          return epochs_in_five_years;
        default:
          return outcome::failure(Errors::kInvalidProofType);
      }
    } else {
      switch (proof) {
        case RegisteredSealProof::kStackedDrg2KiBV1:
        case RegisteredSealProof::kStackedDrg8MiBV1:
        case RegisteredSealProof::kStackedDrg512MiBV1:
        case RegisteredSealProof::kStackedDrg32GiBV1:
        case RegisteredSealProof::kStackedDrg64GiBV1:
          return epochs_in_540_days;
        case RegisteredSealProof::kStackedDrg2KiBV1_1:
        case RegisteredSealProof::kStackedDrg8MiBV1_1:
        case RegisteredSealProof::kStackedDrg512MiBV1_1:
        case RegisteredSealProof::kStackedDrg32GiBV1_1:
        case RegisteredSealProof::kStackedDrg64GiBV1_1:
          return epochs_in_five_years;
        default:
          return outcome::failure(Errors::kInvalidProofType);
      }
    }
  }

  outcome::result<StoragePower> ProofPolicy::getPoStProofConsensusMinerMinPower(
      RegisteredPoStProof proof) const {
    switch (proof) {
      case RegisteredPoStProof::kStackedDRG2KiBWindowPoSt:
      case RegisteredPoStProof::kStackedDRG8MiBWindowPoSt:
      case RegisteredPoStProof::kStackedDRG512MiBWindowPoSt:
      case RegisteredPoStProof::kStackedDRG32GiBWindowPoSt:
      case RegisteredPoStProof::kStackedDRG64GiBWindowPoSt:
        return StoragePower(10) << 40;
      default:
        return outcome::failure(Errors::kInvalidProofType);
    }
  }

}  // namespace fc::vm::actor::builtin::v3::miner
