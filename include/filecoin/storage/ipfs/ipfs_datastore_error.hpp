/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_IPFS_DATASTORE_ERROR_HPP
#define CPP_FILECOIN_CORE_STORAGE_IPFS_DATASTORE_ERROR_HPP

#include "filecoin/common/outcome.hpp"

namespace fc::storage::ipfs {

  /**
   * @brief Type of errors returned by Keystore
   */
  enum class IpfsDatastoreError {
    NOT_FOUND = 1,

    UNKNOWN = 1000
  };

}  // namespace fc::storage::ipfs

OUTCOME_HPP_DECLARE_ERROR(fc::storage::ipfs, IpfsDatastoreError);

#endif  // CPP_FILECOIN_CORE_STORAGE_IPFS_DATASTORE_ERROR_HPP
