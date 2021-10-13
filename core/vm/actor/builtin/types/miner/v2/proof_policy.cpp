/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/v2/proof_policy.hpp"

#include "const.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  using primitives::sector::Errors;

  outcome::result<ChainEpoch> ProofPolicy::getSealProofSectorMaximumLifetime(
      RegisteredSealProof proof, NetworkVersion nv) const {
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
        return 5 * kEpochsInYear;
      default:
        return outcome::failure(Errors::kInvalidProofType);
    }
  }

}  // namespace fc::vm::actor::builtin::v2::miner
