/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_STORAGE_HAMT_HAMT_HPP
#define CPP_FILECOIN_STORAGE_HAMT_HAMT_HPP

#include <string>
#include <vector>

#include <boost/variant.hpp>

#include "common/outcome_throw.hpp"
#include "common/visitor.hpp"
#include "primitives/big_int.hpp"
#include "storage/ipfs/datastore.hpp"

namespace fc::storage::hamt {
  enum class HamtError { EXPECTED_CID = 1, NOT_FOUND, MAX_DEPTH };

  using CID = libp2p::multi::ContentIdentifier;
  using fc::primitives::UBigInt;
  using Value = ipfs::IpfsDatastore::Value;

  extern const CID kDummyCid;

  constexpr size_t kLeafMax = 3;

  struct Node {
    using Ptr = std::shared_ptr<Node>;
    using Leaf = std::map<std::string, Value>;
    using Item = boost::variant<CID, Ptr, Leaf>;

    UBigInt bits;
    std::vector<Item> items;

    static outcome::result<CID> cid(gsl::span<const uint8_t> encoded);
    outcome::result<CID> cid() const;
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &s, const Node &node) {
    auto l_items = s.list();
    for (auto &item : node.items) {
      auto m_item = s.map();
      visit_in_place(
          item,
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
    return s << (s.list() << node.bits << l_items);
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &s, Node &node) {
    auto l_node = s.list();
    l_node >> node.bits;
    auto n_items = l_node.listLength();
    auto l_items = l_node.list();
    for (size_t i = 0; i < n_items; ++i) {
      auto m_item = l_items.map();
      if (m_item.find("0") != m_item.end()) {
        CID cid = kDummyCid;
        m_item.at("0") >> cid;
        node.items.push_back(cid);
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
        node.items.push_back(leaf);
      }
    }
    return s;
  }

  class Hamt {
   public:
    using Visitor = std::function<outcome::result<void>(const std::string &,
                                                        const Value &)>;

    Hamt(std::shared_ptr<ipfs::IpfsDatastore> store, Node::Ptr root);
    Hamt(std::shared_ptr<ipfs::IpfsDatastore> store, const CID &root);
    outcome::result<void> set(const std::string &key,
                              gsl::span<const uint8_t> value);
    outcome::result<Value> get(const std::string &key);
    outcome::result<void> remove(const std::string &key);
    outcome::result<void> flush();
    outcome::result<void> visit(const Visitor &visitor);

   private:
    outcome::result<void> set(Node &node,
                              gsl::span<const size_t> indices,
                              const std::string &key,
                              gsl::span<const uint8_t> value);
    outcome::result<void> remove(Node &node,
                                 gsl::span<const size_t> indices,
                                 const std::string &key);
    outcome::result<void> flush(Node::Item &item);
    outcome::result<void> loadItem(Node::Item &item);
    outcome::result<void> visit(Node::Item &item, const Visitor &visitor);

    std::shared_ptr<ipfs::IpfsDatastore> store_;
    Node::Item root_;
  };
}  // namespace fc::storage::hamt

OUTCOME_HPP_DECLARE_ERROR(fc::storage::hamt, HamtError);

#endif  // CPP_FILECOIN_STORAGE_HAMT_HAMT_HPP
