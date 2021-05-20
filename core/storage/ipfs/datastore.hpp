/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include "codec/cbor/cbor_codec.hpp"
#include "storage/ipfs/ipfs_datastore_error.hpp"

namespace fc::primitives::block {
  struct BlockHeader;
}  // namespace fc::primitives::block

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

    virtual std::shared_ptr<IpfsDatastore> shared() = 0;

    /**
     * @brief CBOR-serialize value and store
     * @param value - data to serialize and store
     * @return cid of CBOR-serialized data
     */
    template <typename T>
    outcome::result<CID> setCbor(const T &value) {
      OUTCOME_TRY(bytes, encode(value));
      OUTCOME_TRY(key, common::getCidOf(bytes));
      OUTCOME_TRY(set(key, std::move(bytes)));
      return std::move(key);
    }

    void setCbor(const primitives::block::BlockHeader &) = delete;

    /// Get CBOR decoded value by CID
    template <typename T>
    outcome::result<T> getCbor(const CID &key) const {
      OUTCOME_TRY(bytes, get(key));
      return decode<T>(bytes);
    }

    template <typename T>
    static outcome::result<Value> encode(const T &value) {
      OUTCOME_TRY(flush(value));
      return codec::cbor::encode(value);
    }

    template <typename T>
    outcome::result<T> decode(gsl::span<const uint8_t> input) const {
      OUTCOME_TRY(value, codec::cbor::decode<T>(input));
      load(value);
      return std::move(value);
    }

    template <typename T>
    void load(T &value) const {
      Load<T>::call(const_cast<IpfsDatastore &>(*this), value);
    }

    template <typename T>
    static auto flush(T &value) {
      using Z = std::remove_const_t<T>;
      return Flush<Z>::call(const_cast<Z &>(value));
    }

    template <typename T>
    struct Visit {
      template <typename Visitor>
      static void call(T &, const Visitor &) {}
    };

    template <typename T>
    struct Load {
      static void call(IpfsDatastore &ipld, T &value) {
        Visit<T>::call(value, [&](auto &x) { ipld.load(x); });
      }
    };

    template <typename T>
    struct Flush {
      static outcome::result<void> call(T &value) {
        try {
          Visit<T>::call(
              value, [](auto &x) { OUTCOME_EXCEPT(IpfsDatastore::flush(x)); });
        } catch (std::system_error &e) {
          return outcome::failure(e.code());
        }
        return outcome::success();
      }
    };
  };
}  // namespace fc::storage::ipfs

namespace fc {
  using Ipld = storage::ipfs::IpfsDatastore;
  using IpldPtr = std::shared_ptr<Ipld>;

  template <typename T>
  struct CIDT : public CID {
    CIDT() : CID() {}
    CIDT(CID &&cid) noexcept : CID(cid) {}
    explicit CIDT(const CID &cid) : CID(cid) {}

    auto get() const {
      return ipld->getCbor<T>(*this);
    }
    outcome::result<void> set(const T &value) {
      OUTCOME_TRYA(*this, ipld->setCbor(value));
      return outcome::success();
    }
    IpldPtr ipld;
  };

  template <typename T>
  struct Ipld::Load<CIDT<T>> {
    static void call(Ipld &ipld, CIDT<T> &cid) {
      cid.ipld = ipld.shared();
    }
  };
}  // namespace fc
