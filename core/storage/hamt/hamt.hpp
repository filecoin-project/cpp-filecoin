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
#include "common/outcome_throw.hpp"
#include "common/visitor.hpp"
#include "primitives/cid/cid.hpp"
#include "storage/ipfs/datastore.hpp"

namespace fc::storage::hamt {
  enum class HamtError { EXPECTED_CID = 1, NOT_FOUND, MAX_DEPTH };
}  // namespace fc::storage::hamt

OUTCOME_HPP_DECLARE_ERROR(fc::storage::hamt, HamtError);

namespace fc::storage::hamt {
  using boost::multiprecision::cpp_int;
  using Value = ipfs::IpfsDatastore::Value;

  constexpr size_t kLeafMax = 3;
  constexpr size_t kDefaultBitWidth = 8;

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
  };

  CBOR_ENCODE(Node, node) {
    auto l_items = s.list();
    Bits bits;
    for (auto &item : node.items) {
      bit_set(bits, item.first);
      auto m_item = s.map();
      visit_in_place(
          item.second,
          [&m_item](const CID &cid) { m_item["0"] << cid; },
          [](const Node::Ptr &ptr) { outcome::raise(HamtError::EXPECTED_CID); },
          [&m_item](const Node::Leaf &leaf) {
            auto &s_leaf = m_item["1"];
            auto l_pairs = s_leaf.list();
            for (auto &pair : leaf) {
              l_pairs << (l_pairs.list()
                          << pair.first << l_pairs.wrap(pair.second, 1));
            }
            s_leaf << l_pairs;
          });
      l_items << m_item;
    }
    return s << (s.list() << bits << l_items);
  }

  CBOR_DECODE(Node, node) {
    node.items.clear();
    auto l_node = s.list();
    Bits bits;
    l_node >> bits;
    auto n_items = l_node.listLength();
    auto l_items = l_node.list();
    size_t j = 0;
    for (size_t i = 0; i < n_items; ++i) {
      while (!bit_test(bits, j)) {
        ++j;
      }
      auto m_item = l_items.map();
      if (m_item.find("0") != m_item.end()) {
        CID cid;
        m_item.at("0") >> cid;
        node.items[j] = std::move(cid);
      } else {
        auto s_leaf = m_item.at("1");
        auto n_leaf = s_leaf.listLength();
        auto l_leaf = s_leaf.list();
        Node::Leaf leaf;
        for (size_t j = 0; j < n_leaf; ++j) {
          auto l_pair = l_leaf.list();
          std::string key;
          l_pair >> key;
          leaf.emplace(key, l_pair.raw());
        }
        node.items[j] = std::move(leaf);
      }
      ++j;
    }
    return s;
  }

  /**
   * Hamt map
   * https://github.com/ipld/specs/blob/c1b0d3f4dc26850071d0e4d67854408e970ed29c/data-structures/hashmap.md
   */
  class Hamt {
   public:
    using Visitor = std::function<outcome::result<void>(const std::string &,
                                                        const Value &)>;

    Hamt(std::shared_ptr<ipfs::IpfsDatastore> store,
         size_t bit_width = kDefaultBitWidth);
    Hamt(std::shared_ptr<ipfs::IpfsDatastore> store, Node::Ptr root);
    Hamt(std::shared_ptr<ipfs::IpfsDatastore> store,
         const CID &root,
         size_t bit_width = kDefaultBitWidth);
    /** Set value by key, does not write to storage */
    outcome::result<void> set(const std::string &key,
                              gsl::span<const uint8_t> value);

    /** Get value by key */
    outcome::result<Value> get(const std::string &key);

    /**
     * Remove value by key, does not write to storage.
     * Returns NOT_FOUND if element doesn't exist.
     */
    outcome::result<void> remove(const std::string &key);

    /**
     * Checks if key is present
     */
    outcome::result<bool> contains(const std::string &key);

    /**
     * Write changes made by set and remove to storage
     * @return new root
     */
    outcome::result<CID> flush();

    /** Apply visitor for key value pairs */
    outcome::result<void> visit(const Visitor &visitor);

    /// Store CBOR encoded value by key
    template <typename T>
    outcome::result<void> setCbor(const std::string &key, const T &value) {
      OUTCOME_TRY(bytes, codec::cbor::encode(value));
      return set(key, bytes);
    }

    /// Get CBOR decoded value by key
    template <typename T>
    outcome::result<T> getCbor(const std::string &key) {
      OUTCOME_TRY(bytes, get(key));
      return codec::cbor::decode<T>(bytes);
    }

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
    outcome::result<void> visit(Node::Item &item, const Visitor &visitor);

    std::shared_ptr<ipfs::IpfsDatastore> store_;
    Node::Item root_;
    size_t bit_width_;
  };
}  // namespace fc::storage::hamt

#endif  // CPP_FILECOIN_STORAGE_HAMT_HAMT_HPP
