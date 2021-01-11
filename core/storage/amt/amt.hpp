/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_STORAGE_AMT_AMT_HPP
#define CPP_FILECOIN_STORAGE_AMT_AMT_HPP

#include <boost/variant.hpp>

#include "codec/cbor/cbor.hpp"
#include "common/outcome.hpp"
#include "common/visitor.hpp"
#include "common/which.hpp"
#include "primitives/cid/cid.hpp"
#include "storage/ipfs/datastore.hpp"

namespace fc::storage::amt {
  enum class AmtError {
    kExpectedCID = 1,
    kDecodeWrong,
    kIndexTooBig,
    kNotFound,
  };
}  // namespace fc::storage::amt

OUTCOME_HPP_DECLARE_ERROR(fc::storage::amt, AmtError);

namespace fc::storage::amt {
  constexpr size_t kWidth = 8;
  constexpr auto kMaxIndex = 1ull << 48;

  using common::which;
  using Value = ipfs::IpfsDatastore::Value;

  struct Node {
    using Ptr = std::shared_ptr<Node>;
    using Link = boost::variant<CID, Ptr>;
    using Links = std::map<size_t, Link>;
    using Values = std::map<size_t, Value>;
    using Items = boost::variant<Values, Links>;

    Items items;
  };

  CBOR_ENCODE(Node, node) {
    std::vector<uint8_t> bits;
    auto l_links = s.list();
    auto l_values = s.list();
    bits.resize(1);
    visit_in_place(
        node.items,
        [&bits, &l_links](const Node::Links &links) {
          for (auto &item : links) {
            bits[0] |= 1 << item.first;
            if (which<Node::Ptr>(item.second)) {
              outcome::raise(AmtError::kExpectedCID);
            }
            l_links << boost::get<CID>(item.second);
          }
        },
        [&bits, &l_values](const Node::Values &values) {
          for (auto &item : values) {
            bits[0] |= 1 << item.first;
            l_values << l_values.wrap(item.second, 1);
          }
        });
    return s << (s.list() << bits << l_links << l_values);
  }

  CBOR_DECODE(Node, node) {
    auto l_node = s.list();
    std::vector<uint8_t> bits;
    l_node >> bits;
    if (bits.size() != 1) {
      outcome::raise(AmtError::kDecodeWrong);
    }
    std::vector<size_t> indices;
    for (auto i = 0u; i < 8; ++i) {
      if (bits[0] & (1 << i)) {
        indices.push_back(i);
      }
    }

    auto n_links = l_node.listLength();
    auto l_links = l_node.list();
    auto n_values = l_node.listLength();
    auto l_values = l_node.list();
    if (n_links != 0 && n_values != 0) {
      outcome::raise(AmtError::kDecodeWrong);
    }
    if (n_links != 0) {
      if (n_links != indices.size()) {
        outcome::raise(AmtError::kDecodeWrong);
      }
      Node::Links links;
      for (auto i = 0u; i < n_links; ++i) {
        CID link;
        l_links >> link;
        links[indices[i]] = std::move(link);
      }
      node.items = links;
    } else {
      if (n_values != indices.size()) {
        outcome::raise(AmtError::kDecodeWrong);
      }
      Node::Values values;
      for (auto i = 0u; i < n_values; ++i) {
        values[indices[i]] = Value{l_values.raw()};
      }
      node.items = values;
    }
    return s;
  }

  struct Root {
    uint64_t height{};
    uint64_t count{};
    Node node;
  };

  CBOR_TUPLE(Root, height, count, node)

  class Amt {
   public:
    using Visitor =
        std::function<outcome::result<void>(uint64_t, const Value &)>;

    explicit Amt(std::shared_ptr<ipfs::IpfsDatastore> store);
    Amt(std::shared_ptr<ipfs::IpfsDatastore> store, const CID &root);
    /// Get values quantity
    outcome::result<uint64_t> count() const;
    /// Set value by key, does not write to storage
    outcome::result<void> set(uint64_t key, gsl::span<const uint8_t> value);
    /// Get value by key
    outcome::result<Value> get(uint64_t key) const;
    /// Remove value by key, does not write to storage
    outcome::result<void> remove(uint64_t key);
    /// Checks if key is present
    outcome::result<bool> contains(uint64_t key) const;
    /// Write changes made by set and remove to storage
    outcome::result<CID> flush();
    /// Get root CID if flushed, throw otherwise
    const CID &cid() const;
    /// Apply visitor for key value pairs
    outcome::result<void> visit(const Visitor &visitor) const;

    /// Store CBOR encoded value by key
    template <typename T>
    outcome::result<void> setCbor(uint64_t key, const T &value) {
      OUTCOME_TRY(bytes, Ipld::encode(value));
      return set(key, bytes);
    }

    /// Get CBOR decoded value by key
    template <typename T>
    outcome::result<T> getCbor(uint64_t key) const {
      OUTCOME_TRY(bytes, get(key));
      return ipld->decode<T>(bytes);
    }

    IpldPtr ipld;

   private:
    outcome::result<bool> set(Node &node,
                              uint64_t height,
                              uint64_t key,
                              gsl::span<const uint8_t> value);
    outcome::result<bool> remove(Node &node, uint64_t height, uint64_t key);
    outcome::result<void> flush(Node &node);
    outcome::result<void> visit(Node &node,
                                uint64_t height,
                                uint64_t offset,
                                const Visitor &visitor) const;
    outcome::result<void> loadRoot() const;
    outcome::result<Node::Ptr> loadLink(Node &node,
                                        uint64_t index,
                                        bool create) const;

    mutable boost::variant<CID, Root> root_;
  };
}  // namespace fc::storage::amt

#endif  // CPP_FILECOIN_STORAGE_AMT_AMT_HPP
