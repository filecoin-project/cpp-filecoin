/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CRYPTO_BLAKE2B_BLAKE2B_ERROR_HPP
#define CPP_FILECOIN_CORE_CRYPTO_BLAKE2B_BLAKE2B_ERROR_HPP

#include "common/outcome.hpp"

namespace fc::crypto::blake2b {

  /**
   * @brief Type of errors returned by blake2b
   */
  enum class Blake2bError {
    OK = 0,
    CANNOT_INIT = 1,
    UNKNOWN = 1000
  };

} // namespace fc::crypto::blake2b

OUTCOME_HPP_DECLARE_ERROR(fc::crypto::blake2b, Blake2bError);

#endif  // CPP_FILECOIN_CORE_CRYPTO_BLAKE2B_BLAKE2B_ERROR_HPP
