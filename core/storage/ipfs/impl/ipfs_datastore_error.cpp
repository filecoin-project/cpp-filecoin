/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipfs/ipfs_datastore_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::ipfs, IpfsDatastoreError, e) {
  using fc::storage::ipfs::IpfsDatastoreError;

  if (e == IpfsDatastoreError::kNotFound) {
    return "IpfsDatastoreError: cid not found";
  }
  return "unknown error";
}
