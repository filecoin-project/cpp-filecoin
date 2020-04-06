/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_MULTIMAP_HPP
#define CPP_FILECOIN_MULTIMAP_HPP

#include "adt/array.hpp"
#include "adt/map.hpp"
#include "storage/hamt/hamt.hpp"

namespace fc::adt {

  /**
   * Container for storing multiple values per key.
   * Implemented over a HAMT of AMTs.
   * The order of values by the same key is preserved.
   */
  struct Multimap {
    using Value = storage::ipfs::IpfsDatastore::Value;
    using Visitor = std::function<outcome::result<void>(const Value &)>;

    explicit Multimap(
        const std::shared_ptr<storage::ipfs::IpfsDatastore> &store);
    Multimap(const std::shared_ptr<storage::ipfs::IpfsDatastore> &store,
             const CID &root);

    /**
     * Apply changes to storage
     * @return root CID
     */
    outcome::result<CID> flush();

    /// Adds a value by key to the end of array. Does not change the store
    outcome::result<void> add(const std::string &key,
                              gsl::span<const uint8_t> value);

    /**
     * Adds a value which can be CBOR-encoded.
     * Does not change the store immediately
     * @tparam T value type that supports CBOR encoding
     * @param value to be marshalled
     * @return operation result
     */
    template <typename T>
    outcome::result<void> addCbor(const std::string &key, const T &value) {
      OUTCOME_TRY(bytes, codec::cbor::encode(value));
      return add(key, bytes);
    }

    /// Removes array under the key
    outcome::result<void> removeAll(const std::string &key);

    /// Iterates over stored values by key
    outcome::result<void> visit(const std::string &key, const Visitor &visitor);

    template <typename Value, typename Keyer, size_t bit_width>
    static outcome::result<void> append(
        Map<Array<Value>, Keyer, bit_width> &map,
        const typename Keyer::Key &key,
        const Value &value) {
      OUTCOME_TRY(array, map.tryGet(key));
      if (!array) {
        array = Array<Value>{};
      }
      array->load(map.hamt.getIpld());
      OUTCOME_TRY(count, array->amt.count());
      OUTCOME_TRY(array->set(count, value));
      OUTCOME_TRY(array->flush());
      return map.set(key, *array);
    }

    template <typename Value, typename Keyer, size_t bit_width>
    static outcome::result<void> visit(
        Map<Array<Value>, Keyer, bit_width> &map,
        const typename Keyer::Key &key,
        const std::function<outcome::result<void>(const Value &)> &visitor) {
      OUTCOME_TRY(array, map.tryGet(key));
      if (array) {
        array->load(map.hamt.getIpld());
        OUTCOME_TRY(
            array->visit([&](auto, auto &value) { return visitor(value); }));
      }
      return outcome::success();
    }

   private:
    std::shared_ptr<storage::ipfs::IpfsDatastore> store_;
    storage::hamt::Hamt hamt_;
  };
}  // namespace fc::adt

#endif  // CPP_FILECOIN_MULTIMAP_HPP
