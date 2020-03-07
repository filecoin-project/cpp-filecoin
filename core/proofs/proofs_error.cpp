/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proofs/proofs_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::proofs, ProofsError, e) {
  using fc::proofs::ProofsError;

  switch (e) {
    case (ProofsError::CANNOT_OPEN_FILE):
      return "Proofs: cannot open file";
    case (ProofsError::NO_SUCH_SEAL_PROOF):
      return "Proofs: No mapping to FFIRegisteredSealProof";
    case (ProofsError::NO_SUCH_POST_PROOF):
      return "Proofs: No mapping to FFIRegisteredPoStProof";
    case (ProofsError::INVALID_POST_PROOF):
      return "Proofs: No mapping to RegisteredPoSt";
    case (ProofsError::UNKNOWN):
      return "Proofs: unknown error";
  }
}
