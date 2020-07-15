/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proofs/proof_param_provider_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::proofs, ProofParamProviderError, e) {
  using fc::proofs::ProofParamProviderError;

  switch (e) {
    case (ProofParamProviderError::kChecksumMismatch):
      return "ParamProvider: checksum mismatch";
    case (ProofParamProviderError::kFileDoesNotOpen):
      return "ParamProvider: file does not open";
    case (ProofParamProviderError::kMissingEntry):
      return "ParamProvider: a required entry is missing in the provided JSON "
             "file";
    case (ProofParamProviderError::kInvalidSectorSize):
      return "ParamProvider: cannot convert sector size";
    case (ProofParamProviderError::kInvalidJSON):
      return "ParamProvider: cannot read json file";
    case (ProofParamProviderError::kInvalidURL):
      return "ParamProvider: can not parse url";
    case (ProofParamProviderError::kFailedDownloadingFile):
      return "ParamProvider: could not download file";
    case (ProofParamProviderError::kCannotCreateDir):
      return "ParamProvider: could not create directory";
    case (ProofParamProviderError::kFailedDownloading):
      return "ParamProvider: errors occurred while downloading files";
  }

  return "unknown error";
}
