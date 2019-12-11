/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_STORAGE_KEYSTORE_ERROR_HPP
#define FILECOIN_CORE_STORAGE_KEYSTORE_ERROR_HPP

#include "common/outcome.hpp"

namespace fc::storage::keystore {

  /**
   * @brief Type of errors returned by Keystore
   */
  enum class KeyStoreError {
    OK = 0,
    NOT_FOUND = 1,
    ALREADY_EXISTS = 2,
    WRONG_ADDRESS = 3,
    WRONG_SIGNATURE = 4,
    CANNOT_STORE = 5,
    CANNOT_READ = 6,

    UNKNOWN = 1000
  };

}  // namespace fc::storage::keystore

OUTCOME_HPP_DECLARE_ERROR(fc::storage::keystore, KeyStoreError);

#endif  // FILECOIN_CORE_STORAGE_KEYSTORE_ERROR_HPP
