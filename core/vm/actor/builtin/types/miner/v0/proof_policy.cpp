/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/v0/proof_policy.hpp"

#include "const.hpp"

namespace fc::vm::actor::builtin::v0::miner {
  using primitives::sector::Errors;

  outcome::result<ChainEpoch> ProofPolicy::getSealProofSectorMaximumLifetime(
      RegisteredSealProof proof, NetworkVersion nv) const {
    switch (proof) {
      case RegisteredSealProof::kStackedDrg2KiBV1:
      case RegisteredSealProof::kStackedDrg8MiBV1:
      case RegisteredSealProof::kStackedDrg512MiBV1:
      case RegisteredSealProof::kStackedDrg32GiBV1:
      case RegisteredSealProof::kStackedDrg64GiBV1:
        return 5 * kEpochsInYear;
      default:
        return outcome::failure(Errors::kInvalidProofType);
    }
  }

  outcome::result<StoragePower> ProofPolicy::getPoStProofConsensusMinerMinPower(
      RegisteredPoStProof proof) const {
    return outcome::failure(Errors::kInvalidProofType);
  }

  outcome::result<uint64_t> ProofPolicy::getPoStProofWindowPoStPartitionSectors(
      RegisteredPoStProof proof) const {
    switch (proof) {
      case RegisteredPoStProof::kStackedDRG2KiBWindowPoSt:
      case RegisteredPoStProof::kStackedDRG8MiBWindowPoSt:
      case RegisteredPoStProof::kStackedDRG512MiBWindowPoSt:
        return 2;
      case RegisteredPoStProof::kStackedDRG32GiBWindowPoSt:
        return 2349;
      case RegisteredPoStProof::kStackedDRG64GiBWindowPoSt:
        return 2300;
      default:
        return outcome::failure(Errors::kInvalidProofType);
    }
  }

}  // namespace fc::vm::actor::builtin::v0::miner
