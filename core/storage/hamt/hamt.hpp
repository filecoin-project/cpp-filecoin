/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_STORAGE_HAMT_HAMT_HPP
#define CPP_FILECOIN_STORAGE_HAMT_HAMT_HPP

#include <string>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/variant.hpp>

#include "codec/cbor/cbor.hpp"
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
  using common::Buffer;
  using Value = ipfs::IpfsDatastore::Value;

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
    using Leaf = std::map<std::string, Value>;
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
    using Visitor = std::function<outcome::result<void>(const std::string &,
                                                        const Value &)>;

    Hamt(std::shared_ptr<ipfs::IpfsDatastore> store, size_t bit_width, bool v3);
    Hamt(std::shared_ptr<ipfs::IpfsDatastore> store,
         Node::Ptr root,
         size_t bit_width,
         bool v3);
    Hamt(std::shared_ptr<ipfs::IpfsDatastore> store,
         const CID &root,
         size_t bit_width,
         bool v3);
    /** Set value by key, does not write to storage */
    outcome::result<void> set(const std::string &key,
                              gsl::span<const uint8_t> value);

    /** Get value by key */
    outcome::result<Value> get(const std::string &key) const;

    /**
     * Remove value by key, does not write to storage.
     * Returns kNotFound if element doesn't exist.
     */
    outcome::result<void> remove(const std::string &key);

    /**
     * Checks if key is present
     */
    outcome::result<bool> contains(const std::string &key) const;

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
    outcome::result<void> loadRoot();

    /// Store CBOR encoded value by key
    template <typename T>
    outcome::result<void> setCbor(const std::string &key, const T &value) {
      OUTCOME_TRY(bytes, Ipld::encode(value));
      return set(key, bytes);
    }

    /// Get CBOR decoded value by key
    template <typename T>
    outcome::result<T> getCbor(const std::string &key) const {
      OUTCOME_TRY(bytes, get(key));
      return ipld->decode<T>(bytes);
    }

    /// Get CBOR decoded value by key
    template <typename T>
    outcome::result<boost::optional<T>> tryGetCbor(
        const std::string &key) const {
      auto maybe = get(key);
      if (!maybe) {
        if (maybe.error() != HamtError::kNotFound) {
          return maybe.error();
        }
        return boost::none;
      }
      OUTCOME_TRY(value, ipld->decode<T>(maybe.value()));
      return std::move(value);
    }

    IpldPtr ipld;

   private:
    std::vector<size_t> keyToIndices(const std::string &key, int n = -1) const;
    outcome::result<void> set(Node &node,
                              gsl::span<const size_t> indices,
                              const std::string &key,
                              gsl::span<const uint8_t> value);
    outcome::result<void> remove(Node &node,
                                 gsl::span<const size_t> indices,
                                 const std::string &key);
    static outcome::result<void> cleanShard(Node::Item &item);
    outcome::result<void> flush(Node::Item &item);
    outcome::result<void> loadItem(Node::Item &item) const;
    outcome::result<void> visit(Node::Item &item, const Visitor &visitor) const;

    mutable Node::Item root_;
    size_t bit_width_;
    bool v3_;
  };
}  // namespace fc::storage::hamt

#endif  // CPP_FILECOIN_STORAGE_HAMT_HAMT_HPP
