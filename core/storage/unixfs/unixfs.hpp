/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <iosfwd>

#include "storage/ipfs/datastore.hpp"

namespace fc::storage::unixfs {
  constexpr size_t kMaxLinks = 1024;
  constexpr size_t kChunkSize{1 << 20};

  outcome::result<CID> wrapFile(Ipld &ipld,
                                std::ifstream &file,
                                size_t chunk_size = kChunkSize,
                                size_t max_links = kMaxLinks);

  /**
   * Stores data as DAG
   * @param ipld - data storage
   * @param data - to srore
   * @param chunk_size
   * @param max_links
   * @return root CID
   */
  outcome::result<CID> wrapFile(Ipld &ipld,
                                gsl::span<const uint8_t> data,
                                size_t chunk_size = kChunkSize,
                                size_t max_links = kMaxLinks);
}  // namespace fc::storage::unixfs
