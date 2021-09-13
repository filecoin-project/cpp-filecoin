/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"

namespace fc::storage::ipfs {

  /**
   * @brief Type of errors returned by Keystore
   */
  enum class IpfsDatastoreError {
    kNotFound = 1,
  };

}  // namespace fc::storage::ipfs

OUTCOME_HPP_DECLARE_ERROR(fc::storage::ipfs, IpfsDatastoreError);
