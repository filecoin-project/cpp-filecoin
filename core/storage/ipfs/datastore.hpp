/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_IPFS_DATASTORE_HPP
#define CPP_FILECOIN_CORE_STORAGE_IPFS_DATASTORE_HPP

#include <libp2p/multi/content_identifier.hpp>
#include <vector>

#include "codec/cbor/cbor.hpp"
#include "common/buffer.hpp"
#include "common/logger.hpp"
#include "common/outcome.hpp"
#include "crypto/blake2/blake2b160.hpp"
#include "storage/ipfs/ipfs_datastore_error.hpp"

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

    /// Compute CID from bytes
    inline static outcome::result<CID> cid(gsl::span<const uint8_t> bytes) {
      OUTCOME_TRY(hash_raw, crypto::blake2b::blake2b_256(bytes));
      OUTCOME_TRY(hash,
                  libp2p::multi::Multihash::create(
                      libp2p::multi::HashType::blake2b_256, hash_raw));
      return CID(CID::Version::V1, libp2p::multi::MulticodecType::DAG_CBOR, hash);
    }

    /// Compute CID from CBOR encoded value
    template <typename T>
    static outcome::result<CID> cidCbor(const T &value) {
      OUTCOME_TRY(bytes, codec::cbor::encode(value));
      return cid(bytes);
    }

    /// Store CBOR encoded value
    template <typename T>
    outcome::result<CID> setCbor(const T &value) {
      OUTCOME_TRY(bytes, codec::cbor::encode(value));
      OUTCOME_TRY(key, cid(bytes));
      OUTCOME_TRY(set(key, Value(bytes)));
      return std::move(key);
    }

    /// Get CBOR encoded value by CID
    template <typename T>
    outcome::result<T> getCbor(const CID &key) {
      OUTCOME_TRY(bytes, get(key));
      return codec::cbor::decode<T>(bytes);
    }
  };
}  // namespace fc::storage::ipfs

#endif  // CPP_FILECOIN_CORE_STORAGE_IPFS_DATASTORE_HPP
