/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/sector/sector.hpp"

#include "common/error_text.hpp"

namespace fc::primitives::sector {
  outcome::result<RegisteredPoStProof> getRegisteredWindowPoStProof(
      RegisteredSealProof proof) {
    switch (proof) {
      case RegisteredSealProof::kStackedDrg64GiBV1:
      case RegisteredSealProof::kStackedDrg64GiBV1_1:
        return RegisteredPoStProof::kStackedDRG64GiBWindowPoSt;
      case RegisteredSealProof::kStackedDrg32GiBV1:
      case RegisteredSealProof::kStackedDrg32GiBV1_1:
        return RegisteredPoStProof::kStackedDRG32GiBWindowPoSt;
      case RegisteredSealProof::kStackedDrg512MiBV1:
      case RegisteredSealProof::kStackedDrg512MiBV1_1:
        return RegisteredPoStProof::kStackedDRG512MiBWindowPoSt;
      case RegisteredSealProof::kStackedDrg8MiBV1:
      case RegisteredSealProof::kStackedDrg8MiBV1_1:
        return RegisteredPoStProof::kStackedDRG8MiBWindowPoSt;
      case RegisteredSealProof::kStackedDrg2KiBV1:
      case RegisteredSealProof::kStackedDrg2KiBV1_1:
        return RegisteredPoStProof::kStackedDRG2KiBWindowPoSt;
      case RegisteredSealProof::kUndefined:
        return RegisteredPoStProof::kUndefined;
      default:
        return Errors::kInvalidSealProof;
    }
  }

  outcome::result<RegisteredPoStProof> getRegisteredWinningPoStProof(
      RegisteredSealProof proof) {
    switch (proof) {
      case RegisteredSealProof::kStackedDrg64GiBV1:
      case RegisteredSealProof::kStackedDrg64GiBV1_1:
        return RegisteredPoStProof::kStackedDRG64GiBWinningPoSt;
      case RegisteredSealProof::kStackedDrg32GiBV1:
      case RegisteredSealProof::kStackedDrg32GiBV1_1:
        return RegisteredPoStProof::kStackedDRG32GiBWinningPoSt;
      case RegisteredSealProof::kStackedDrg512MiBV1:
      case RegisteredSealProof::kStackedDrg512MiBV1_1:
        return RegisteredPoStProof::kStackedDRG512MiBWinningPoSt;
      case RegisteredSealProof::kStackedDrg8MiBV1:
      case RegisteredSealProof::kStackedDrg8MiBV1_1:
        return RegisteredPoStProof::kStackedDRG8MiBWinningPoSt;
      case RegisteredSealProof::kStackedDrg2KiBV1:
      case RegisteredSealProof::kStackedDrg2KiBV1_1:
        return RegisteredPoStProof::kStackedDRG2KiBWinningPoSt;
      case RegisteredSealProof::kUndefined:
        return RegisteredPoStProof::kUndefined;
      default:
        return Errors::kInvalidSealProof;
    }
  }

  outcome::result<RegisteredPoStProof> getRegisteredWinningPoStProof(
      RegisteredPoStProof proof) {
    switch (proof) {
      case RegisteredPoStProof::kStackedDRG64GiBWinningPoSt:
      case RegisteredPoStProof::kStackedDRG64GiBWindowPoSt:
        return RegisteredPoStProof::kStackedDRG64GiBWinningPoSt;
      case RegisteredPoStProof::kStackedDRG32GiBWinningPoSt:
      case RegisteredPoStProof::kStackedDRG32GiBWindowPoSt:
        return RegisteredPoStProof::kStackedDRG32GiBWinningPoSt;
      case RegisteredPoStProof::kStackedDRG512MiBWinningPoSt:
      case RegisteredPoStProof::kStackedDRG512MiBWindowPoSt:
        return RegisteredPoStProof::kStackedDRG512MiBWinningPoSt;
      case RegisteredPoStProof::kStackedDRG8MiBWinningPoSt:
      case RegisteredPoStProof::kStackedDRG8MiBWindowPoSt:
        return RegisteredPoStProof::kStackedDRG8MiBWinningPoSt;
      case RegisteredPoStProof::kStackedDRG2KiBWinningPoSt:
      case RegisteredPoStProof::kStackedDRG2KiBWindowPoSt:
        return RegisteredPoStProof::kStackedDRG2KiBWinningPoSt;
      case RegisteredPoStProof::kUndefined:
        return RegisteredPoStProof::kUndefined;
      default:
        return Errors::kInvalidPoStProof;
    }
  }

  outcome::result<RegisteredUpdateProof> getRegisteredUpdateProof(
      RegisteredSealProof proof) {
    switch (proof) {
      case RegisteredSealProof::kStackedDrg64GiBV1:
      case RegisteredSealProof::kStackedDrg64GiBV1_1:
        return RegisteredUpdateProof::kStackedDrg64GiBV1;
      case RegisteredSealProof::kStackedDrg32GiBV1:
      case RegisteredSealProof::kStackedDrg32GiBV1_1:
        return RegisteredUpdateProof::kStackedDrg32GiBV1;
      case RegisteredSealProof::kStackedDrg512MiBV1:
      case RegisteredSealProof::kStackedDrg512MiBV1_1:
        return RegisteredUpdateProof::kStackedDrg512MiBV1;
      case RegisteredSealProof::kStackedDrg8MiBV1:
      case RegisteredSealProof::kStackedDrg8MiBV1_1:
        return RegisteredUpdateProof::kStackedDrg8MiBV1;
      case RegisteredSealProof::kStackedDrg2KiBV1:
      case RegisteredSealProof::kStackedDrg2KiBV1_1:
        return RegisteredUpdateProof::kStackedDrg2KiBV1;
      case RegisteredSealProof::kUndefined:
        return RegisteredUpdateProof::kUndefined;
      default:
        return Errors::kInvalidSealProof;
    }
  }

  outcome::result<SectorSize> getSectorSize(RegisteredSealProof proof) {
    switch (proof) {
      case RegisteredSealProof::kStackedDrg64GiBV1:
      case RegisteredSealProof::kStackedDrg64GiBV1_1:
        return SectorSize{64} << 30;
      case RegisteredSealProof::kStackedDrg32GiBV1:
      case RegisteredSealProof::kStackedDrg32GiBV1_1:
        return SectorSize{32} << 30;
      case RegisteredSealProof::kStackedDrg512MiBV1:
      case RegisteredSealProof::kStackedDrg512MiBV1_1:
        return SectorSize{512} << 20;
      case RegisteredSealProof::kStackedDrg8MiBV1:
      case RegisteredSealProof::kStackedDrg8MiBV1_1:
        return SectorSize{8} << 20;
      case RegisteredSealProof::kStackedDrg2KiBV1:
      case RegisteredSealProof::kStackedDrg2KiBV1_1:
        return SectorSize{2} << 10;
      case RegisteredSealProof::kUndefined:
        return 0;
      default:
        return Errors::kInvalidSealProof;
    }
  }

  outcome::result<SectorSize> getSectorSize(RegisteredPoStProof proof) {
    switch (proof) {
      case RegisteredPoStProof::kStackedDRG64GiBWinningPoSt:
      case RegisteredPoStProof::kStackedDRG64GiBWindowPoSt:
        return SectorSize{64} << 30;
      case RegisteredPoStProof::kStackedDRG32GiBWinningPoSt:
      case RegisteredPoStProof::kStackedDRG32GiBWindowPoSt:
        return SectorSize{32} << 30;
      case RegisteredPoStProof::kStackedDRG512MiBWinningPoSt:
      case RegisteredPoStProof::kStackedDRG512MiBWindowPoSt:
        return SectorSize{512} << 20;
      case RegisteredPoStProof::kStackedDRG8MiBWinningPoSt:
      case RegisteredPoStProof::kStackedDRG8MiBWindowPoSt:
        return SectorSize{8} << 20;
      case RegisteredPoStProof::kStackedDRG2KiBWinningPoSt:
      case RegisteredPoStProof::kStackedDRG2KiBWindowPoSt:
        return SectorSize{2} << 10;
      case RegisteredPoStProof::kUndefined:
        return 0;
      default:
        return Errors::kInvalidPoStProof;
    }
  }

  outcome::result<size_t> getWindowPoStPartitionSectors(
      RegisteredPoStProof proof) {
    switch (proof) {
      case RegisteredPoStProof::kStackedDRG64GiBWinningPoSt:
      case RegisteredPoStProof::kStackedDRG64GiBWindowPoSt:
        return 2300;
      case RegisteredPoStProof::kStackedDRG32GiBWinningPoSt:
      case RegisteredPoStProof::kStackedDRG32GiBWindowPoSt:
        return 2349;
      case RegisteredPoStProof::kStackedDRG512MiBWinningPoSt:
      case RegisteredPoStProof::kStackedDRG512MiBWindowPoSt:
        return 2;
      case RegisteredPoStProof::kStackedDRG8MiBWinningPoSt:
      case RegisteredPoStProof::kStackedDRG8MiBWindowPoSt:
        return 2;
      case RegisteredPoStProof::kStackedDRG2KiBWinningPoSt:
      case RegisteredPoStProof::kStackedDRG2KiBWindowPoSt:
        return 2;
      case RegisteredPoStProof::kUndefined:
        return 0;
      default:
        return Errors::kInvalidPoStProof;
    }
  }

  outcome::result<size_t> getSealProofWindowPoStPartitionSectors(
      RegisteredSealProof proof) {
    OUTCOME_TRY(wpost_proof_type, getRegisteredWindowPoStProof(proof));
    return getWindowPoStPartitionSectors(wpost_proof_type);
  }

  outcome::result<RegisteredSealProof>
  getPreferredSealProofTypeFromWindowPoStType(NetworkVersion network_version,
                                              RegisteredPoStProof proof) {
    // support for the new proofs in network added in version 7, and removed
    // support for the old ones in network version 8.
    if (network_version < NetworkVersion::kVersion7) {
      switch (proof) {
        case RegisteredPoStProof::kStackedDRG2KiBWindowPoSt:
          return RegisteredSealProof::kStackedDrg2KiBV1;
        case RegisteredPoStProof::kStackedDRG8MiBWindowPoSt:
          return RegisteredSealProof::kStackedDrg8MiBV1;
        case RegisteredPoStProof::kStackedDRG512MiBWindowPoSt:
          return RegisteredSealProof::kStackedDrg512MiBV1;
        case RegisteredPoStProof::kStackedDRG32GiBWindowPoSt:
          return RegisteredSealProof::kStackedDrg32GiBV1;
        case RegisteredPoStProof::kStackedDRG64GiBWindowPoSt:
          return RegisteredSealProof::kStackedDrg64GiBV1;
        default:
          return Errors::kInvalidPoStProof;
      }
    }

    switch (proof) {
      case RegisteredPoStProof::kStackedDRG2KiBWindowPoSt:
        return RegisteredSealProof::kStackedDrg2KiBV1_1;
      case RegisteredPoStProof::kStackedDRG8MiBWindowPoSt:
        return RegisteredSealProof::kStackedDrg8MiBV1_1;
      case RegisteredPoStProof::kStackedDRG512MiBWindowPoSt:
        return RegisteredSealProof::kStackedDrg512MiBV1_1;
      case RegisteredPoStProof::kStackedDRG32GiBWindowPoSt:
        return RegisteredSealProof::kStackedDrg32GiBV1_1;
      case RegisteredPoStProof::kStackedDRG64GiBWindowPoSt:
        return RegisteredSealProof::kStackedDrg64GiBV1_1;
      default:
        return Errors::kInvalidPoStProof;
    }
  }
}  // namespace fc::primitives::sector

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
