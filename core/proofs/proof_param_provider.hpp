/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/logger.hpp"
#include "common/outcome.hpp"
#include "gsl/span"
#include "proof_param_provider_error.hpp"
#include "proofs/proof_param_provider_error.hpp"

namespace fc::proofs {

  struct ParamFile {
    std::string cid;
    std::string digest;
    uint64_t sector_size = 0;
  };

  outcome::result<void> getParams(
      const std::map<std::string, ParamFile> &param_files,
      uint64_t storage_size);

}  // namespace fc::proofs
