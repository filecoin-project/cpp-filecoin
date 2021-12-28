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
    case (ProofsError::kFileDoesntExist):
      return "Proofs: unsealed file doesn't exist";
    case (ProofsError::kNotReadEnough):
      return "Proofs: cannot read chunk";
    case (ProofsError::kNotWriteEnough):
      return "Proofs: cannot write chunk";
    case (ProofsError::kCannotCreateUnsealedFile):
      return "Proofs: cannot create unsealed file";
    case (ProofsError::kCannotCreatePipe):
      return "Proofs: cannot create pipe";
    case (ProofsError::kCannotWriteData):
      return "Proofs: cannot write data from piece";
    case (ProofsError::kUnclassifiedError):
      return "Proofs: Unclassified ffi error";
    case (ProofsError::kCallerError):
      return "Proofs: ffi error from caller side";
    case (ProofsError::kReceiverError):
      return "Proofs: ffi error from receiver side";
    case (ProofsError::kNoSuchAggregationSealProof):
      return "Proofs: No mapping to FFIAggregationSealProof";
    case (ProofsError::kNoSuchUpdateProof):
      return "Proofs: No mapping to FFIUpdateProof";
    default:
      return "Proofs: unknown error";
  }
}
