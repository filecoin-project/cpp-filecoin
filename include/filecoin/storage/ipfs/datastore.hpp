/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_IPFS_DATASTORE_HPP
#define CPP_FILECOIN_CORE_STORAGE_IPFS_DATASTORE_HPP

#include <vector>

#include "filecoin/codec/cbor/cbor.hpp"
#include "filecoin/common/buffer.hpp"
#include "filecoin/common/logger.hpp"
#include "filecoin/common/outcome.hpp"
#include "filecoin/primitives/cid/cid.hpp"
#include "filecoin/storage/ipfs/ipfs_datastore_error.hpp"

namespace fc::storage::ipfs {

  class IpfsDatastore {
   public:
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

    /**
     * @brief removes key from data store
     * @param key key to remove
     * @return success if removed or didn't exist, error otherwise
     */
    virtual outcome::result<void> remove(const CID &key) = 0;

    /**
     * @brief CBOR-serialize value and store
     * @param value - data to serialize and store
     * @return cid of CBOR-serialized data
     */
    template <typename T>
    outcome::result<CID> setCbor(const T &value) {
      OUTCOME_TRY(bytes, codec::cbor::encode(value));
      OUTCOME_TRY(key, common::getCidOf(bytes));
      OUTCOME_TRY(set(key, Value(bytes)));
      return std::move(key);
    }

    /// Get CBOR decoded value by CID
    template <typename T>
    outcome::result<T> getCbor(const CID &key) const {
      OUTCOME_TRY(bytes, get(key));
      return codec::cbor::decode<T>(bytes);
    }
  };
}  // namespace fc::storage::ipfs

#endif  // CPP_FILECOIN_CORE_STORAGE_IPFS_DATASTORE_HPP
