/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_ARRAY_HPP
#define CPP_FILECOIN_ARRAY_HPP

#include "codec/cbor/cbor.hpp"
#include "common/outcome.hpp"
#include "primitives/cid/cid.hpp"
#include "storage/amt/amt.hpp"
#include "storage/ipfs/datastore.hpp"

namespace fc::adt {

  /**
   * Container for storing values preserving their order of insertion.
   * Implementation is based on Array Mapped Trie.
   */
  struct Array {
    using Value = storage::ipfs::IpfsDatastore::Value;
    using IndexedVisitor =
        std::function<outcome::result<void>(uint64_t, const Value &)>;
    using Visitor = std::function<outcome::result<void>(const Value &)>;

    Array(const std::shared_ptr<storage::ipfs::IpfsDatastore> &store);
    Array(const std::shared_ptr<storage::ipfs::IpfsDatastore> &store,
          const CID &root);

    /**
     * Apply changes to storage
     * @return root CID
     */
    outcome::result<CID> flush();

    /// Appends a value. Does not change the store
    outcome::result<void> append(gsl::span<const uint8_t> value);

    /**
     * Appends a value which can be CBOR-encoded.
     * Does not change the store immediately
     * @tparam T value type that supports CBOR encoding
     * @param value to be marshalled
     * @return operation result
     */
    template <typename T>
    outcome::result<void> appendCbor(const T &value) {
      OUTCOME_TRY(bytes, codec::cbor::encode(value));
      return append(bytes);
    }

    /// Iterate over stored elements acquiring their index
    outcome::result<void> visit(const IndexedVisitor &visitor);

    /// Iterate over stored elements
    outcome::result<void> visit(const Visitor &visitor);

   private:
    std::shared_ptr<storage::ipfs::IpfsDatastore> store_;
    storage::amt::Amt amt_;
  };

}  // namespace fc::adt

#endif  // CPP_FILECOIN_ARRAY_HPP
