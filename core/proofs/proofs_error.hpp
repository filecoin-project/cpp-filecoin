/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_PROOFS_PROOFS_ERROR_HPP
#define FILECOIN_CORE_PROOFS_PROOFS_ERROR_HPP

#include "common/outcome.hpp"

namespace fc::proofs {

  /**
   * @brief Proofs engine returns these types of errors
   */
  enum class ProofsError {
    kCannotOpenFile = 1,
    kNoSuchSealProof,
    kNoSuchPostProof,
    kInvalidPostProof,
    kOutOfBound,
    kUnableMoveCursor,
    kFileDoesntExist,
    kNotReadEnough,
    kNotWriteEnough,
    kCannotCreateUnsealedFile,
    kUnknown = 1000
  };

}  // namespace fc::proofs

OUTCOME_HPP_DECLARE_ERROR(fc::proofs, ProofsError);

#endif  // FILECOIN_CORE_PROOFS_PROOFS_ERROR_HPP
