/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_IPFS_DATASTORE_HPP
#define CPP_FILECOIN_CORE_STORAGE_IPFS_DATASTORE_HPP

#include <libp2p/multi/content_identifier.hpp>
#include <vector>

#include "common/buffer.hpp"
#include "common/logger.hpp"
#include "common/outcome.hpp"

namespace fc::storage::ipfs {

  class IpfsDatastore {
   public:
    using CID = libp2p::multi::ContentIdentifier;
    using Value = common::Buffer;

    virtual ~IpfsDatastore() = default;

    /**
     * @brief check if data store has value
     * @param key key to find
     * @return true if value exists, false otherwise
     */
    virtual bool contains(const CID &key) const = 0;

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

    /**
     * @brief removes key from data store
     * @param key key to remove
     * @return success if removed or didn't exist, error otherwise
     */
    virtual outcome::result<void> remove(const CID &key) = 0;
  };

}  // namespace fc::storage::ipfs

#endif  // CPP_FILECOIN_CORE_STORAGE_IPFS_DATASTORE_HPP
