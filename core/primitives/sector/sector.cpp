/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/sector/sector.hpp"

namespace fc::primitives::sector {
  outcome::result<RegisteredProof> getRegisteredPoStProof(
      RegisteredProof proof) {
    switch (proof) {
      case RegisteredProof::StackedDRG32GiBSeal:
      case RegisteredProof::StackedDRG32GiBPoSt:
        return RegisteredProof::StackedDRG32GiBPoSt;
      case RegisteredProof::StackedDRG2KiBSeal:
      case RegisteredProof::StackedDRG2KiBPoSt:
        return RegisteredProof::StackedDRG2KiBPoSt;
      case RegisteredProof::StackedDRG8MiBSeal:
      case RegisteredProof::StackedDRG8MiBPoSt:
        return RegisteredProof::StackedDRG8MiBPoSt;
      case RegisteredProof::StackedDRG512MiBSeal:
      case RegisteredProof::StackedDRG512MiBPoSt:
        return RegisteredProof::StackedDRG512MiBPoSt;
      default:
        return Errors::InvalidPoStProof;
    }
  }

  outcome::result<RegisteredProof> getRegisteredSealProof(
      RegisteredProof proof) {
    switch (proof) {
      case RegisteredProof::StackedDRG32GiBSeal:
      case RegisteredProof::StackedDRG32GiBPoSt:
        return RegisteredProof::StackedDRG32GiBSeal;
      case RegisteredProof::StackedDRG2KiBSeal:
      case RegisteredProof::StackedDRG2KiBPoSt:
        return RegisteredProof::StackedDRG2KiBSeal;
      case RegisteredProof::StackedDRG8MiBSeal:
      case RegisteredProof::StackedDRG8MiBPoSt:
        return RegisteredProof::StackedDRG8MiBSeal;
      case RegisteredProof::StackedDRG512MiBSeal:
      case RegisteredProof::StackedDRG512MiBPoSt:
        return RegisteredProof::StackedDRG512MiBSeal;
      default:
        return Errors::InvalidSealProof;
    }
  }

  outcome::result<SectorSize> getSectorSize(RegisteredProof proof) {
    switch (proof) {
      case RegisteredProof::StackedDRG32GiBSeal:
      case RegisteredProof::StackedDRG32GiBPoSt:
        return SectorSize{32} << 30;
      case RegisteredProof::StackedDRG2KiBSeal:
      case RegisteredProof::StackedDRG2KiBPoSt:
        return SectorSize{2} << 10;
      case RegisteredProof::StackedDRG8MiBSeal:
      case RegisteredProof::StackedDRG8MiBPoSt:
        return SectorSize{8} << 20;
      case RegisteredProof::StackedDRG512MiBSeal:
      case RegisteredProof::StackedDRG512MiBPoSt:
        return SectorSize{512} << 20;
      default:
        return Errors::InvalidProofType;
    }
  }

};  // namespace fc::primitives::sector

OUTCOME_CPP_DEFINE_CATEGORY(fc::primitives::sector, Errors, e) {
  using fc::primitives::sector::Errors;
  switch (e) {
    case (Errors::InvalidSealProof):
      return "Sector: unsupported mapping to Seal-specific RegisteredProof";
    case (Errors::InvalidPoStProof):
      return "Sector: unsupported mapping to PoSt-specific RegisteredProof";
    case (Errors::InvalidProofType):
      return "Sector: unsupported proof type";
    default:
      return "Sector: unknown error";
  }
}
