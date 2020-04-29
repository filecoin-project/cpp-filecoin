/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_UNIXFS_UNIXFS_HPP
#define CPP_FILECOIN_CORE_STORAGE_UNIXFS_UNIXFS_HPP

#include "storage/ipfs/datastore.hpp"

namespace fc::storage::unixfs {
  using Ipld = ipfs::IpfsDatastore;

  outcome::result<CID> wrapFile(Ipld &ipld, gsl::span<const uint8_t> data);
}  // namespace fc::storage::unixfs

#endif  // CPP_FILECOIN_CORE_STORAGE_UNIXFS_UNIXFS_HPP
