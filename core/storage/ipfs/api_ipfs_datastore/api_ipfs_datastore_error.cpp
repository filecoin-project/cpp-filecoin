/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipfs/api_ipfs_datastore/api_ipfs_datastore_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::ipfs, ApiIpfsDatastoreError, e) {
  using E = fc::storage::ipfs::ApiIpfsDatastoreError;
  if (e == E::kNotSupproted) {
    return "ApiIpfsDatastoreError: operation is not supported";
  }
  return "ApiIpfsDatastoreError: unknown error";
}
