/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/sector/sector.hpp"

namespace fc::primitives::sector {
  outcome::result<RegisteredProof> getRegisteredWindowPoStProof(
      RegisteredProof proof) {
    switch (proof) {
      case RegisteredProof::StackedDRG64GiBWinningPoSt:
      case RegisteredProof::StackedDRG64GiBWindowPoSt:
        return RegisteredProof::StackedDRG64GiBWindowPoSt;
      case RegisteredProof::StackedDRG32GiBWinningPoSt:
      case RegisteredProof::StackedDRG32GiBWindowPoSt:
        return RegisteredProof::StackedDRG32GiBWindowPoSt;
      case RegisteredProof::StackedDRG512MiBWinningPoSt:
      case RegisteredProof::StackedDRG512MiBWindowPoSt:
        return RegisteredProof::StackedDRG512MiBWindowPoSt;
      case RegisteredProof::StackedDRG8MiBWinningPoSt:
      case RegisteredProof::StackedDRG8MiBWindowPoSt:
        return RegisteredProof::StackedDRG8MiBWindowPoSt;
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
      case RegisteredProof::StackedDRG64GiBWinningPoSt:
      case RegisteredProof::StackedDRG64GiBWindowPoSt:
        return RegisteredProof::StackedDRG64GiBWinningPoSt;
      case RegisteredProof::StackedDRG32GiBWinningPoSt:
      case RegisteredProof::StackedDRG32GiBWindowPoSt:
        return RegisteredProof::StackedDRG32GiBWinningPoSt;
      case RegisteredProof::StackedDRG512MiBWinningPoSt:
      case RegisteredProof::StackedDRG512MiBWindowPoSt:
        return RegisteredProof::StackedDRG512MiBWinningPoSt;
      case RegisteredProof::StackedDRG8MiBWinningPoSt:
      case RegisteredProof::StackedDRG8MiBWindowPoSt:
        return RegisteredProof::StackedDRG8MiBWinningPoSt;
      case RegisteredProof::StackedDRG2KiBWinningPoSt:
      case RegisteredProof::StackedDRG2KiBWindowPoSt:
        return RegisteredProof::StackedDRG2KiBWinningPoSt;
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

  outcome::result<SectorSize> getSectorSize(RegisteredProof proof) {
    switch (proof) {
      case RegisteredProof::StackedDRG64GiBWinningPoSt:
      case RegisteredProof::StackedDRG64GiBWindowPoSt:
        return SectorSize{64} << 30;
      case RegisteredProof::StackedDRG32GiBWinningPoSt:
      case RegisteredProof::StackedDRG32GiBWindowPoSt:
        return SectorSize{32} << 30;
      case RegisteredProof::StackedDRG512MiBWinningPoSt:
      case RegisteredProof::StackedDRG512MiBWindowPoSt:
        return SectorSize{512} << 20;
      case RegisteredProof::StackedDRG8MiBWinningPoSt:
      case RegisteredProof::StackedDRG8MiBWindowPoSt:
        return SectorSize{8} << 20;
      case RegisteredProof::StackedDRG2KiBWinningPoSt:
      case RegisteredProof::StackedDRG2KiBWindowPoSt:
        return SectorSize{2} << 10;
      default:
        return Errors::kInvalidPoStProof;
    }
  }

  outcome::result<size_t> getWindowPoStPartitionSectors(RegisteredProof proof) {
    switch (proof) {
      case RegisteredProof::StackedDRG64GiBWinningPoSt:
      case RegisteredProof::StackedDRG64GiBWindowPoSt:
        return 2300;
      case RegisteredProof::StackedDRG32GiBWinningPoSt:
      case RegisteredProof::StackedDRG32GiBWindowPoSt:
        return 2349;
      case RegisteredProof::StackedDRG512MiBWinningPoSt:
      case RegisteredProof::StackedDRG512MiBWindowPoSt:
        return 2;
      case RegisteredProof::StackedDRG8MiBWinningPoSt:
      case RegisteredProof::StackedDRG8MiBWindowPoSt:
        return 2;
      case RegisteredProof::StackedDRG2KiBWinningPoSt:
      case RegisteredProof::StackedDRG2KiBWindowPoSt:
        return 2;
      default:
        return Errors::kInvalidPoStProof;
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
