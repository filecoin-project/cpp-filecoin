/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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
