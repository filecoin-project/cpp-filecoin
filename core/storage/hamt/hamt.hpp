/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/variant.hpp>

#include "codec/cbor/cbor_codec.hpp"
#include "codec/cbor/streams_annotation.hpp"
#include "common/outcome.hpp"
#include "common/span.hpp"
#include "common/visitor.hpp"
#include "primitives/cid/cid.hpp"
#include "storage/ipfs/datastore.hpp"

namespace fc::storage::hamt {
  enum class HamtError {
    kExpectedCID = 1,
    kNotFound,
    kMaxDepth,
    kInconsistent,
  };
}  // namespace fc::storage::hamt

OUTCOME_HPP_DECLARE_ERROR(fc::storage::hamt, HamtError);

namespace fc::storage::hamt {
  using boost::multiprecision::cpp_int;

  constexpr size_t kLeafMax = 3;
  constexpr size_t kDefaultBitWidth = 5;

  struct Bits : cpp_int {};

  CBOR_ENCODE(Bits, bits) {
    std::vector<uint8_t> bytes;
    if (bits != 0) {
      export_bits(bits, std::back_inserter(bytes), 8);
    }
    return s << bytes;
  }

  CBOR_DECODE(Bits, bits) {
    std::vector<uint8_t> bytes;
    s >> bytes;
    if (bytes.empty()) {
      bits = {0};
    } else {
      import_bits(bits, bytes.begin(), bytes.end());
    }
    return s;
  }

  /** Hamt node representation */
  struct Node {
    using Ptr = std::shared_ptr<Node>;
    using Leaf = std::vector<std::pair<Bytes, Bytes>>;
    using Item = boost::variant<CID, Ptr, Leaf>;

    std::map<size_t, Item> items;
    boost::optional<bool> v3;
  };
  CBOR2_DECODE_ENCODE(Node)

  // TODO(turuslan): v3 caching
  /**
   * Hamt map
   * https://github.com/ipld/specs/blob/c1b0d3f4dc26850071d0e4d67854408e970ed29c/data-structures/hashmap.md
   */
  class Hamt {
   public:
    using Visitor = std::function<outcome::result<void>(BytesIn, BytesIn)>;

    Hamt(std::shared_ptr<ipfs::IpfsDatastore> store, size_t bit_width);
    Hamt(std::shared_ptr<ipfs::IpfsDatastore> store,
         Node::Ptr root,
         size_t bit_width);
    Hamt(std::shared_ptr<ipfs::IpfsDatastore> store,
         const CID &root,
         size_t bit_width);
    /** Set value by key, does not write to storage */
    outcome::result<void> set(BytesIn key, BytesCow &&value);

    /** Get value by key */
    outcome::result<Bytes> get(BytesIn key) const;

    /**
     * Remove value by key, does not write to storage.
     * Returns kNotFound if element doesn't exist.
     */
    outcome::result<void> remove(BytesIn key);

    /**
     * Checks if key is present
     */
    outcome::result<bool> contains(BytesIn key) const;

    /**
     * Write changes made by set and remove to storage
     * @return new root
     */
    outcome::result<CID> flush();

    /** Get root CID if flushed, throw otherwise */
    const CID &cid() const;

    /** Apply visitor for key value pairs */
    outcome::result<void> visit(const Visitor &visitor) const;

    /** Loads root item */
    outcome::result<void> loadRoot() const;

    /// Store CBOR encoded value by key
    template <typename T>
    outcome::result<void> setCbor(BytesIn key, const T &value) {
      OUTCOME_TRY(bytes, cbor_blake::cbEncodeT(value));
      return set(key, std::move(bytes));
    }

    /// Get CBOR decoded value by key
    template <typename T>
    outcome::result<T> getCbor(BytesIn key) const {
      OUTCOME_TRY(bytes, get(key));
      return cbor_blake::cbDecodeT<T>(ipld_, bytes);
    }

    /// Get CBOR decoded value by key
    template <typename T>
    outcome::result<boost::optional<T>> tryGetCbor(BytesIn key) const {
      auto maybe = get(key);
      if (!maybe) {
        if (maybe.error() != HamtError::kNotFound) {
          return maybe.error();
        }
        return boost::none;
      }
      OUTCOME_TRY(value, cbor_blake::cbDecodeT<T>(ipld_, maybe.value()));
      return std::move(value);
    }

    inline IpldPtr getIpld() const {
      return ipld_;
    }

    inline void setIpld(IpldPtr new_ipld) {
      ipld_ = std::move(new_ipld);
    }

   private:
    std::vector<size_t> keyToIndices(BytesIn key, int n = -1) const;
    outcome::result<void> set(Node &node,
                              gsl::span<const size_t> indices,
                              BytesIn key,
                              BytesCow &&value);
    outcome::result<void> remove(Node &node,
                                 gsl::span<const size_t> indices,
                                 BytesIn key);
    static outcome::result<void> cleanShard(Node::Item &item);
    outcome::result<void> flush(Node::Item &item);
    outcome::result<void> loadItem(Node::Item &item) const;
    outcome::result<void> visit(Node::Item &item, const Visitor &visitor) const;

    void lazyCreateRoot() const;
    bool v3() const;

    IpldPtr ipld_;
    mutable Node::Item root_;
    size_t bit_width_{};
  };
}  // namespace fc::storage::hamt
