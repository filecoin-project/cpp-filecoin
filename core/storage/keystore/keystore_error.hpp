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
    kNotFound = 1,
    kAlreadyExists,
    kWrongAddress,
    kWrongSignature,
    kCannotStore,
    kCannotRead,
  };

}  // namespace fc::storage::keystore

OUTCOME_HPP_DECLARE_ERROR(fc::storage::keystore, KeyStoreError);

#endif  // FILECOIN_CORE_STORAGE_KEYSTORE_ERROR_HPP
