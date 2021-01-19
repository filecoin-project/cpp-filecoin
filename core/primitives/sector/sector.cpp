/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/sector/sector.hpp"

namespace fc::primitives::sector {
  outcome::result<RegisteredPoStProof> getRegisteredWindowPoStProof(
      RegisteredSealProof proof) {
    switch (proof) {
      case RegisteredSealProof::StackedDrg64GiBV1:
      case RegisteredSealProof::StackedDrg64GiBV1_1:
        return RegisteredPoStProof::StackedDRG64GiBWindowPoSt;
      case RegisteredSealProof::StackedDrg32GiBV1:
      case RegisteredSealProof::StackedDrg32GiBV1_1:
        return RegisteredPoStProof::StackedDRG32GiBWindowPoSt;
      case RegisteredSealProof::StackedDrg512MiBV1:
      case RegisteredSealProof::StackedDrg512MiBV1_1:
        return RegisteredPoStProof::StackedDRG512MiBWindowPoSt;
      case RegisteredSealProof::StackedDrg8MiBV1:
      case RegisteredSealProof::StackedDrg8MiBV1_1:
        return RegisteredPoStProof::StackedDRG8MiBWindowPoSt;
      case RegisteredSealProof::StackedDrg2KiBV1:
      case RegisteredSealProof::StackedDrg2KiBV1_1:
        return RegisteredPoStProof::StackedDRG2KiBWindowPoSt;
      default:
        return Errors::kInvalidPoStProof;
    }
  }

  outcome::result<RegisteredPoStProof> getRegisteredWinningPoStProof(
      RegisteredSealProof proof) {
    switch (proof) {
      case RegisteredSealProof::StackedDrg64GiBV1:
      case RegisteredSealProof::StackedDrg64GiBV1_1:
        return RegisteredPoStProof::StackedDRG64GiBWinningPoSt;
      case RegisteredSealProof::StackedDrg32GiBV1:
      case RegisteredSealProof::StackedDrg32GiBV1_1:
        return RegisteredPoStProof::StackedDRG32GiBWinningPoSt;
      case RegisteredSealProof::StackedDrg512MiBV1:
      case RegisteredSealProof::StackedDrg512MiBV1_1:
        return RegisteredPoStProof::StackedDRG512MiBWinningPoSt;
      case RegisteredSealProof::StackedDrg8MiBV1:
      case RegisteredSealProof::StackedDrg8MiBV1_1:
        return RegisteredPoStProof::StackedDRG8MiBWinningPoSt;
      case RegisteredSealProof::StackedDrg2KiBV1:
      case RegisteredSealProof::StackedDrg2KiBV1_1:
        return RegisteredPoStProof::StackedDRG2KiBWinningPoSt;
      default:
        return Errors::kInvalidPoStProof;
    }
  }

  outcome::result<SectorSize> getSectorSize(RegisteredSealProof proof) {
    switch (proof) {
      case RegisteredSealProof::StackedDrg64GiBV1:
      case RegisteredSealProof::StackedDrg64GiBV1_1:
        return SectorSize{64} << 30;
      case RegisteredSealProof::StackedDrg32GiBV1:
      case RegisteredSealProof::StackedDrg32GiBV1_1:
        return SectorSize{32} << 30;
      case RegisteredSealProof::StackedDrg512MiBV1:
      case RegisteredSealProof::StackedDrg512MiBV1_1:
        return SectorSize{512} << 20;
      case RegisteredSealProof::StackedDrg8MiBV1:
      case RegisteredSealProof::StackedDrg8MiBV1_1:
        return SectorSize{8} << 20;
      case RegisteredSealProof::StackedDrg2KiBV1:
      case RegisteredSealProof::StackedDrg2KiBV1_1:
        return SectorSize{2} << 10;
      default:
        return Errors::kInvalidPoStProof;
    }
  }

  outcome::result<SectorSize> getSectorSize(RegisteredPoStProof proof) {
    switch (proof) {
      case RegisteredPoStProof::StackedDRG64GiBWinningPoSt:
      case RegisteredPoStProof::StackedDRG64GiBWindowPoSt:
        return SectorSize{64} << 30;
      case RegisteredPoStProof::StackedDRG32GiBWinningPoSt:
      case RegisteredPoStProof::StackedDRG32GiBWindowPoSt:
        return SectorSize{32} << 30;
      case RegisteredPoStProof::StackedDRG512MiBWinningPoSt:
      case RegisteredPoStProof::StackedDRG512MiBWindowPoSt:
        return SectorSize{512} << 20;
      case RegisteredPoStProof::StackedDRG8MiBWinningPoSt:
      case RegisteredPoStProof::StackedDRG8MiBWindowPoSt:
        return SectorSize{8} << 20;
      case RegisteredPoStProof::StackedDRG2KiBWinningPoSt:
      case RegisteredPoStProof::StackedDRG2KiBWindowPoSt:
        return SectorSize{2} << 10;
      default:
        return Errors::kInvalidPoStProof;
    }
  }

  outcome::result<size_t> getWindowPoStPartitionSectors(
      RegisteredPoStProof proof) {
    switch (proof) {
      case RegisteredPoStProof::StackedDRG64GiBWinningPoSt:
      case RegisteredPoStProof::StackedDRG64GiBWindowPoSt:
        return 2300;
      case RegisteredPoStProof::StackedDRG32GiBWinningPoSt:
      case RegisteredPoStProof::StackedDRG32GiBWindowPoSt:
        return 2349;
      case RegisteredPoStProof::StackedDRG512MiBWinningPoSt:
      case RegisteredPoStProof::StackedDRG512MiBWindowPoSt:
        return 2;
      case RegisteredPoStProof::StackedDRG8MiBWinningPoSt:
      case RegisteredPoStProof::StackedDRG8MiBWindowPoSt:
        return 2;
      case RegisteredPoStProof::StackedDRG2KiBWinningPoSt:
      case RegisteredPoStProof::StackedDRG2KiBWindowPoSt:
        return 2;
      default:
        return Errors::kInvalidPoStProof;
    }
  }

  outcome::result<size_t> getSealProofWindowPoStPartitionSectors(
      RegisteredSealProof proof) {
    OUTCOME_TRY(wpost_proof_type, getRegisteredWindowPoStProof(proof));
    return getWindowPoStPartitionSectors(wpost_proof_type);
  }
};  // namespace fc::primitives::sector

OUTCOME_CPP_DEFINE_CATEGORY(fc::primitives::sector, Errors, e) {
  using fc::primitives::sector::Errors;
  switch (e) {
    case (Errors::kInvalidSealProof):
      return "Sector: unsupported mapping to Seal-specific RegisteredSealProof";
    case (Errors::kInvalidPoStProof):
      return "Sector: unsupported mapping to PoSt-specific RegisteredSealProof";
    case (Errors::kInvalidProofType):
      return "Sector: unsupported proof type";
    default:
      return "Sector: unknown error";
  }
}
