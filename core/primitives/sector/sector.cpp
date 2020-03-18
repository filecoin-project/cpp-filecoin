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
        return RegisteredProof::StackedDRG32GiBPoSt;
      case RegisteredProof::StackedDRG32GiBPoSt:
        return RegisteredProof::StackedDRG32GiBPoSt;
      case RegisteredProof::StackedDRG2KiBSeal:
        return RegisteredProof::StackedDRG2KiBPoSt;
      case RegisteredProof::StackedDRG2KiBPoSt:
        return RegisteredProof::StackedDRG2KiBPoSt;
      case RegisteredProof::StackedDRG8MiBSeal:
        return RegisteredProof::StackedDRG8MiBPoSt;
      case RegisteredProof::StackedDRG8MiBPoSt:
        return RegisteredProof::StackedDRG8MiBPoSt;
      case RegisteredProof::StackedDRG512MiBSeal:
        return RegisteredProof::StackedDRG512MiBPoSt;
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
        return RegisteredProof::StackedDRG32GiBSeal;
      case RegisteredProof::StackedDRG32GiBPoSt:
        return RegisteredProof::StackedDRG32GiBSeal;
      case RegisteredProof::StackedDRG2KiBSeal:
        return RegisteredProof::StackedDRG2KiBSeal;
      case RegisteredProof::StackedDRG2KiBPoSt:
        return RegisteredProof::StackedDRG2KiBSeal;
      case RegisteredProof::StackedDRG8MiBSeal:
        return RegisteredProof::StackedDRG8MiBSeal;
      case RegisteredProof::StackedDRG8MiBPoSt:
        return RegisteredProof::StackedDRG8MiBSeal;
      case RegisteredProof::StackedDRG512MiBSeal:
        return RegisteredProof::StackedDRG512MiBSeal;
      case RegisteredProof::StackedDRG512MiBPoSt:
        return RegisteredProof::StackedDRG512MiBSeal;
      default:
        return Errors::InvalidSealProof;
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
    default:
      return "Sector: unknown error";
  }
}
