/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/storage/ipfs/ipfs_datastore_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::ipfs, IpfsDatastoreError, e) {
  using fc::storage::ipfs::IpfsDatastoreError;

  switch (e) {
    case IpfsDatastoreError::NOT_FOUND:
      return "KeyStoreError: address not found";
    case IpfsDatastoreError::UNKNOWN:
      break;
  }

  return "unknown error";
}
