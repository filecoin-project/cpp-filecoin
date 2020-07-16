/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proofs/proofs_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::proofs, ProofsError, e) {
  using fc::proofs::ProofsError;

  switch (e) {
    case (ProofsError::kCannotOpenFile):
      return "Proofs: cannot open file";
    case (ProofsError::kNoSuchSealProof):
      return "Proofs: No mapping to FFIRegisteredSealProof";
    case (ProofsError::kNoSuchPostProof):
      return "Proofs: No mapping to FFIRegisteredPoStProof";
    case (ProofsError::kInvalidPostProof):
      return "Proofs: No mapping to RegisteredPoSt";
    case (ProofsError::kOutOfBound):
      return "Proofs: Piece will exceed the maximum size";
    case (ProofsError::kUnableMoveCursor):
      return "Proofs: Unable to move cursor to write piece";
    default:
      return "Proofs: unknown error";
  }
}
