/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include "codec/cbor/cbor_codec.hpp"
#include "storage/ipfs/ipfs_datastore_error.hpp"
#include "vm/actor/version.hpp"

namespace fc::storage::ipfs {

  struct IpfsDatastore : vm::actor::WithActorVersion {
    using Value = common::Buffer;

    virtual ~IpfsDatastore() = default;

    /**
     * @brief check if data store has value
     * @param key key to find
     * @return true if value exists, false otherwise
     */
    virtual outcome::result<bool> contains(const CID &key) const = 0;

    /**
     * @brief associates key with value in data store
     * @param key key to associate
     * @param value value to associate with key
     * @return success if operation succeeded, error otherwise
     */
    virtual outcome::result<void> set(const CID &key, Value value) = 0;

    /**
     * @brief searches for a key in data store
     * @param key key to find
     * @return value associated with key or error
     */
    virtual outcome::result<Value> get(const CID &key) const = 0;
  };
}  // namespace fc::storage::ipfs

namespace fc {
  using Ipld = storage::ipfs::IpfsDatastore;
  using IpldPtr = std::shared_ptr<Ipld>;
}  // namespace fc

// TODO(turuslan): refactor includes
#include "cbor_blake/ipld_cbor.hpp"
