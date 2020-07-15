/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/sector/sector.hpp"

namespace fc::primitives::sector {
  outcome::result<RegisteredProof> getRegisteredWindowPoStProof(
      RegisteredProof proof) {
    switch (proof) {
      case RegisteredProof::StackedDRG64GiBSeal:
      case RegisteredProof::StackedDRG64GiBWinningPoSt:
      case RegisteredProof::StackedDRG64GiBWindowPoSt:
        return RegisteredProof::StackedDRG64GiBWindowPoSt;
      case RegisteredProof::StackedDRG32GiBSeal:
      case RegisteredProof::StackedDRG32GiBWinningPoSt:
      case RegisteredProof::StackedDRG32GiBWindowPoSt:
        return RegisteredProof::StackedDRG32GiBWindowPoSt;
      case RegisteredProof::StackedDRG512MiBSeal:
      case RegisteredProof::StackedDRG512MiBWinningPoSt:
      case RegisteredProof::StackedDRG512MiBWindowPoSt:
        return RegisteredProof::StackedDRG512MiBWindowPoSt;
      case RegisteredProof::StackedDRG8MiBSeal:
      case RegisteredProof::StackedDRG8MiBWinningPoSt:
      case RegisteredProof::StackedDRG8MiBWindowPoSt:
        return RegisteredProof::StackedDRG8MiBWindowPoSt;
      case RegisteredProof::StackedDRG2KiBSeal:
      case RegisteredProof::StackedDRG2KiBWinningPoSt:
      case RegisteredProof::StackedDRG2KiBWindowPoSt:
        return RegisteredProof::StackedDRG2KiBWindowPoSt;
      default:
        return Errors::kInvalidPoStProof;
    }
  }

  outcome::result<RegisteredProof> getRegisteredWinningPoStProof(
      RegisteredProof proof) {
    switch (proof) {
      case RegisteredProof::StackedDRG64GiBSeal:
      case RegisteredProof::StackedDRG64GiBWinningPoSt:
      case RegisteredProof::StackedDRG64GiBWindowPoSt:
        return RegisteredProof::StackedDRG64GiBWinningPoSt;
      case RegisteredProof::StackedDRG32GiBSeal:
      case RegisteredProof::StackedDRG32GiBWinningPoSt:
      case RegisteredProof::StackedDRG32GiBWindowPoSt:
        return RegisteredProof::StackedDRG32GiBWinningPoSt;
      case RegisteredProof::StackedDRG512MiBSeal:
      case RegisteredProof::StackedDRG512MiBWinningPoSt:
      case RegisteredProof::StackedDRG512MiBWindowPoSt:
        return RegisteredProof::StackedDRG512MiBWinningPoSt;
      case RegisteredProof::StackedDRG8MiBSeal:
      case RegisteredProof::StackedDRG8MiBWinningPoSt:
      case RegisteredProof::StackedDRG8MiBWindowPoSt:
        return RegisteredProof::StackedDRG8MiBWinningPoSt;
      case RegisteredProof::StackedDRG2KiBSeal:
      case RegisteredProof::StackedDRG2KiBWinningPoSt:
      case RegisteredProof::StackedDRG2KiBWindowPoSt:
        return RegisteredProof::StackedDRG2KiBWinningPoSt;
      default:
        return Errors::kInvalidPoStProof;
    }
  }

  outcome::result<RegisteredProof> getRegisteredSealProof(
      RegisteredProof proof) {
    switch (proof) {
      case RegisteredProof::StackedDRG64GiBSeal:
      case RegisteredProof::StackedDRG64GiBWinningPoSt:
      case RegisteredProof::StackedDRG64GiBWindowPoSt:
        return RegisteredProof::StackedDRG64GiBSeal;
      case RegisteredProof::StackedDRG32GiBSeal:
      case RegisteredProof::StackedDRG32GiBWinningPoSt:
      case RegisteredProof::StackedDRG32GiBWindowPoSt:
        return RegisteredProof::StackedDRG32GiBSeal;
      case RegisteredProof::StackedDRG512MiBSeal:
      case RegisteredProof::StackedDRG512MiBWinningPoSt:
      case RegisteredProof::StackedDRG512MiBWindowPoSt:
        return RegisteredProof::StackedDRG512MiBSeal;
      case RegisteredProof::StackedDRG8MiBSeal:
      case RegisteredProof::StackedDRG8MiBWinningPoSt:
      case RegisteredProof::StackedDRG8MiBWindowPoSt:
        return RegisteredProof::StackedDRG8MiBSeal;
      case RegisteredProof::StackedDRG2KiBSeal:
      case RegisteredProof::StackedDRG2KiBWinningPoSt:
      case RegisteredProof::StackedDRG2KiBWindowPoSt:
        return RegisteredProof::StackedDRG2KiBSeal;
      default:
        return Errors::kInvalidSealProof;
    }
  }

  outcome::result<SectorSize> getSectorSize(RegisteredProof proof) {
    OUTCOME_TRY(seal_proof, getRegisteredSealProof(proof));
    switch (seal_proof) {
      case RegisteredProof::StackedDRG64GiBSeal:
        return SectorSize{64} << 30;
      case RegisteredProof::StackedDRG32GiBSeal:
        return SectorSize{32} << 30;
      case RegisteredProof::StackedDRG2KiBSeal:
        return SectorSize{2} << 10;
      case RegisteredProof::StackedDRG8MiBSeal:
        return SectorSize{8} << 20;
      case RegisteredProof::StackedDRG512MiBSeal:
        return SectorSize{512} << 20;
      default:
        return Errors::kInvalidProofType;
    }
  }

  outcome::result<RegisteredProof> sealProofTypeFromSectorSize(
      SectorSize size) {
    switch (size) {
      case SectorSize{64} << 30:
        return RegisteredProof::StackedDRG64GiBSeal;
      case SectorSize{32} << 30:
        return RegisteredProof::StackedDRG32GiBSeal;
      case SectorSize{2} << 10:
        return RegisteredProof::StackedDRG2KiBSeal;
      case SectorSize{8} << 20:
        return RegisteredProof::StackedDRG8MiBSeal;
      case SectorSize{512} << 20:
        return RegisteredProof::StackedDRG512MiBSeal;
      default:
        return Errors::kInvalidProofType;
    }
  }

  outcome::result<size_t> getWindowPoStPartitionSectors(RegisteredProof proof) {
    OUTCOME_TRY(seal_proof, getRegisteredSealProof(proof));
    switch (seal_proof) {
      case RegisteredProof::StackedDRG64GiBSeal:
        return 2300;
      case RegisteredProof::StackedDRG32GiBSeal:
        return 2349;
      case RegisteredProof::StackedDRG2KiBSeal:
        return 2;
      case RegisteredProof::StackedDRG8MiBSeal:
        return 2;
      case RegisteredProof::StackedDRG512MiBSeal:
        return 2;
      default:
        return Errors::kInvalidProofType;
    }
  }
};  // namespace fc::primitives::sector

OUTCOME_CPP_DEFINE_CATEGORY(fc::primitives::sector, Errors, e) {
  using fc::primitives::sector::Errors;
  switch (e) {
    case (Errors::kInvalidSealProof):
      return "Sector: unsupported mapping to Seal-specific RegisteredProof";
    case (Errors::kInvalidPoStProof):
      return "Sector: unsupported mapping to PoSt-specific RegisteredProof";
    case (Errors::kInvalidProofType):
      return "Sector: unsupported proof type";
    default:
      return "Sector: unknown error";
  }
}
