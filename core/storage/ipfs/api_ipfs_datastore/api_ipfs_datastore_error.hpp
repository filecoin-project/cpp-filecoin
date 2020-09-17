/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_STORAGE_IPFS_API_IPFS_DATASTORE_API_IPFS_DATASTORE_ERROR_HPP
#define CPP_FILECOIN_STORAGE_IPFS_API_IPFS_DATASTORE_API_IPFS_DATASTORE_ERROR_HPP

#include "common/outcome.hpp"

namespace fc::storage::ipfs {

  enum class ApiIpfsDatastoreError {
    kNotSupproted = 1,
  };

}

OUTCOME_HPP_DECLARE_ERROR(fc::storage::ipfs, ApiIpfsDatastoreError);

#endif  // CPP_FILECOIN_STORAGE_IPFS_API_IPFS_DATASTORE_API_IPFS_DATASTORE_ERROR_HPP
