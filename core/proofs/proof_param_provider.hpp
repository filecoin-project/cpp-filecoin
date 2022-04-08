/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/logger.hpp"
#include "common/outcome.hpp"
#include "gsl/span"
#include "proofs/proof_param_provider_error.hpp"

namespace fc::proofs {

  outcome::result<void> getParams(const std::string &param_file,
                                  uint64_t storage_size);

}  // namespace fc::proofs
