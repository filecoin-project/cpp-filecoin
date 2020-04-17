/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proofs/proof_param_provider_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::proofs, ProofParamProviderError, e) {
  using fc::proofs::ProofParamProviderError;

  switch (e) {
    case (ProofParamProviderError::CHECKSUM_MISMATCH):
      return "ParamProvider: checksum mismatch";
    case (ProofParamProviderError::FILE_DOES_NOT_OPEN):
      return "ParamProvider: file does not open";
    case (ProofParamProviderError::MISSING_ENTRY):
      return "ParamProvider: a required entry is missing in the provided JSON "
             "file";
    case (ProofParamProviderError::INVALID_SECTOR_SIZE):
      return "ParamProvider: cannot convert sector size";
    case (ProofParamProviderError::INVALID_JSON):
      return "ParamProvider: cannot read json file";
    case (ProofParamProviderError::INVALID_URL):
      return "ParamProvider: can not parse url";
    case (ProofParamProviderError::FAILED_DOWNLOADING_FILE):
      return "ParamProvider: could not download file";
    case (ProofParamProviderError::CANNOT_CREATE_DIR):
      return "ParamProvider: could not create directory";
    case (ProofParamProviderError::FAILED_DOWNLOADING):
      return "ParamProvider: errors occurred while downloading files";
  }

  return "unknown error";
}
