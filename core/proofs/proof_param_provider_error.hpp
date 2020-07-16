/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_PROOFS_PROOF_PARAM_PROVIDER_ERROR_HPP
#define FILECOIN_CORE_PROOFS_PROOF_PARAM_PROVIDER_ERROR_HPP

#include "common/outcome.hpp"

namespace fc::proofs {

  /**
   * @brief Proof Param Provider returns these types of errors
   */
  enum class ProofParamProviderError {
    kChecksumMismatch = 1,
    kFileDoesNotOpen,
    kMissingEntry,
    kInvalidSectorSize,
    kInvalidJSON,
    kInvalidURL,
    kFailedDownloadingFile,
    kCannotCreateDir,
    kFailedDownloading,
  };

}  // namespace fc::proofs

OUTCOME_HPP_DECLARE_ERROR(fc::proofs, ProofParamProviderError);

#endif  // FILECOIN_CORE_PROOFS_PROOF_PARAM_PROVIDER_ERROR_HPP
