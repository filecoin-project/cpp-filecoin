/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/hamt/hamt.hpp"

#include <libp2p/crypto/sha/sha256.hpp>

#include "common/which.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::hamt, HamtError, e) {
  using fc::storage::hamt::HamtError;
  switch (e) {
    case HamtError::kExpectedCID:
      return "Expected CID";
    case HamtError::kNotFound:
      return "Not found";
    case HamtError::kMaxDepth:
      return "Max depth exceeded";
    case HamtError::kInconsistent:
      return "HamtError::kInconsistent";
  }
  return "Unknown error";
}

namespace fc::storage::hamt {
  using common::which;

  CBOR2_ENCODE(Node) {
    Bits bits;
    auto l_items{s.list()};
    for (auto &[index, value] : v.items) {
      bit_set(bits, index);
      if (boost::get<Node::Ptr>(&value)) {
        outcome::raise(HamtError::kExpectedCID);
      }
      codec::cbor::CborEncodeStream _item;
      auto _cid{boost::get<CID>(&value)};
      if (_cid) {
        _item << *_cid;
      } else {
        auto &leaf{boost::get<Node::Leaf>(value)};
        _item = s.list();
        for (auto &[key, value] : leaf) {
          _item << (s.list() << key << s.wrap(value, 1));
        }
      }
      if (*v.v3) {
        l_items << _item;
      } else {
        auto m_item{s.map()};
        m_item[_cid ? "0" : "1"] << _item;
        l_items << m_item;
      }
    }
    return s << (s.list() << bits << l_items);
  }

  CBOR2_DECODE(Node) {
    v.items.clear();
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
      auto _item{l_items};
      l_items.next();
      auto v3{true};
      if (_item.isMap()) {
        v3 = false;
        auto m_item{_item.map()};
        if (m_item.empty()) {
          outcome::raise(HamtError::kInconsistent);
        }
        _item = m_item.begin()->second;
      }
      if (!v.v3) {
        v.v3 = v3;
      } else if (v3 != *v.v3) {
        outcome::raise(HamtError::kInconsistent);
      }
      if (_item.isCid()) {
        v.items[j] = _item.get<CID>();
      } else {
        auto n_leaf{_item.listLength()};
        auto l_leaf{_item.list()};
        Node::Leaf leaf;
        for (size_t j = 0; j < n_leaf; ++j) {
          auto l_pair{l_leaf.list()};
          auto key{l_pair.get<Bytes>()};
          leaf.emplace(std::move(key), l_pair.raw());
        }
        v.items[j] = std::move(leaf);
      }
      ++j;
    }
    return s;
  }

  auto consumeIndex(gsl::span<const size_t> indices) {
    return indices.subspan(1);
  }

  Hamt::Hamt(std::shared_ptr<ipfs::IpfsDatastore> store,
             size_t bit_width,
             bool v3)
      : ipld{std::move(store)},
        root_{std::make_shared<Node>(Node{{}, v3})},
        bit_width_{bit_width},
        v3_{v3} {}

  Hamt::Hamt(std::shared_ptr<ipfs::IpfsDatastore> store,
             Node::Ptr root,
             size_t bit_width,
             bool v3)
      : ipld{std::move(store)},
        root_{std::move(root)},
        bit_width_{bit_width},
        v3_{v3} {}

  Hamt::Hamt(std::shared_ptr<ipfs::IpfsDatastore> store,
             const CID &root,
             size_t bit_width,
             bool v3)
      : ipld{std::move(store)}, root_{root}, bit_width_{bit_width}, v3_{v3} {}

  outcome::result<void> Hamt::set(BytesIn key, BytesIn value) {
    OUTCOME_TRY(loadItem(root_));
    return set(*boost::get<Node::Ptr>(root_), keyToIndices(key), key, value);
  }

  outcome::result<Bytes> Hamt::get(BytesIn key) const {
    OUTCOME_TRY(loadItem(root_));
    auto node = boost::get<Node::Ptr>(root_);
    for (auto index : keyToIndices(key)) {
      auto it = node->items.find(index);
      if (it == node->items.end()) {
        return HamtError::kNotFound;
      }
      auto &item = it->second;
      OUTCOME_TRY(loadItem(item));
      if (which<Node::Ptr>(item)) {
        node = boost::get<Node::Ptr>(item);
      } else {
        auto &leaf = boost::get<Node::Leaf>(item);
        auto it{leaf.find(key)};
        if (it == leaf.end()) {
          return HamtError::kNotFound;
        }
        return it->second;
      }
    }
    return HamtError::kMaxDepth;
  }

  outcome::result<void> Hamt::remove(BytesIn key) {
    OUTCOME_TRY(loadItem(root_));
    return remove(*boost::get<Node::Ptr>(root_), keyToIndices(key), key);
  }

  outcome::result<bool> Hamt::contains(BytesIn key) const {
    auto res = get(key);
    if (!res) {
      if (res.error() == HamtError::kNotFound) return false;
      return res.error();
    }
    return true;
  }

  outcome::result<CID> Hamt::flush() {
    OUTCOME_TRY(flush(root_));
    return cid();
  }

  const CID &Hamt::cid() const {
    return boost::get<CID>(root_);
  }

  std::vector<size_t> Hamt::keyToIndices(BytesIn key, int n) const {
    std::vector<uint8_t> key_bytes(key.begin(), key.end());
    auto hash = libp2p::crypto::sha256(key_bytes);
    std::vector<size_t> indices;
    constexpr auto byte_bits = 8;
    auto max_bits = byte_bits * hash.size();
    max_bits -= max_bits % bit_width_;
    auto offset = 0;
    if (n != -1) {
      offset = max_bits - (n - 1) * bit_width_;
    }
    while (offset + bit_width_ <= max_bits) {
      size_t index = 0;
      for (auto i = 0u; i < bit_width_; ++i, ++offset) {
        index <<= 1;
        index |= 1
                 & (hash[offset / byte_bits]
                    >> (byte_bits - 1 - offset % byte_bits));
      }
      indices.push_back(index);
    }
    return indices;
  }

  outcome::result<void> Hamt::set(Node &node,
                                  gsl::span<const size_t> indices,
                                  BytesIn key,
                                  BytesIn value) {
    if (indices.empty()) {
      return HamtError::kMaxDepth;
    }
    auto index = indices[0];
    auto it = node.items.find(index);
    if (it == node.items.end()) {
      node.items[index] = Node::Leaf{{copy(key), copy(value)}};
      return outcome::success();
    }
    auto &item = it->second;
    OUTCOME_TRY(loadItem(item));
    if (which<Node::Ptr>(item)) {
      return set(
          *boost::get<Node::Ptr>(item), consumeIndex(indices), key, value);
    }
    auto &leaf = boost::get<Node::Leaf>(item);
    if (auto it2{leaf.find(key)}; it2 != leaf.end()) {
      copy(it2->second, value);
    } else if (leaf.size() < kLeafMax) {
      leaf.emplace(copy(key), copy(value));
    } else {
      auto child = std::make_shared<Node>();
      child->v3 = v3_;
      OUTCOME_TRY(set(*child, consumeIndex(indices), key, value));
      for (auto &pair : leaf) {
        auto indices2 = keyToIndices(pair.first, indices.size());
        OUTCOME_TRY(set(*child, indices2, pair.first, pair.second));
      }
      item = child;
    }
    return outcome::success();
  }

  outcome::result<void> Hamt::remove(Node &node,
                                     gsl::span<const size_t> indices,
                                     BytesIn key) {
    if (indices.empty()) {
      return HamtError::kMaxDepth;
    }
    auto index = indices[0];
    auto it = node.items.find(index);
    if (it == node.items.end()) {
      return HamtError::kNotFound;
    }
    auto &item = it->second;
    OUTCOME_TRY(loadItem(item));
    if (which<Node::Ptr>(item)) {
      OUTCOME_TRY(
          remove(*boost::get<Node::Ptr>(item), consumeIndex(indices), key));
      OUTCOME_TRY(cleanShard(item));
    } else {
      auto &leaf = boost::get<Node::Leaf>(item);
      if (leaf.find(key) == leaf.end()) {
        return HamtError::kNotFound;
      }
      if (leaf.size() == 1) {
        node.items.erase(index);
      } else {
        leaf.erase(leaf.find(key));
      }
    }
    return outcome::success();
  }

  outcome::result<void> Hamt::cleanShard(Node::Item &item) {
    auto &node = *boost::get<Node::Ptr>(item);
    if (node.items.size() == 1) {
      auto &single_item = node.items.begin()->second;
      if (which<Node::Leaf>(single_item)) {
        item = single_item;
      }
    } else if (node.items.size() <= kLeafMax) {
      Node::Leaf leaf;
      for (auto &item2 : node.items) {
        if (!which<Node::Leaf>(item2.second)) {
          return outcome::success();
        }
        for (auto &pair : boost::get<Node::Leaf>(item2.second)) {
          leaf.emplace(pair);
          if (leaf.size() > kLeafMax) {
            return outcome::success();
          }
        }
      }
      item = leaf;
    }
    return outcome::success();
  }

  outcome::result<void> Hamt::flush(Node::Item &item) {
    if (which<Node::Ptr>(item)) {
      auto &node = *boost::get<Node::Ptr>(item);
      for (auto &item2 : node.items) {
        OUTCOME_TRY(flush(item2.second));
      }
      OUTCOME_TRYA(item, fc::setCbor(ipld, node));
    }
    return outcome::success();
  }

  outcome::result<void> Hamt::loadRoot() {
    return loadItem(root_);
  }

  outcome::result<void> Hamt::loadItem(Node::Item &item) const {
    if (which<CID>(item)) {
      OUTCOME_TRY(child, fc::getCbor<Node>(ipld, boost::get<CID>(item)));
      if (!child.v3) {
        child.v3 = v3_;
      } else if (*child.v3 != v3_) {
        return HamtError::kInconsistent;
      }
      item = std::make_shared<Node>(std::move(child));
    }
    return outcome::success();
  }

  outcome::result<void> Hamt::visit(const Visitor &visitor) const {
    return visit(root_, visitor);
  }

  outcome::result<void> Hamt::visit(Node::Item &item,
                                    const Visitor &visitor) const {
    OUTCOME_TRY(loadItem(item));
    if (which<Node::Ptr>(item)) {
      for (auto &item2 : boost::get<Node::Ptr>(item)->items) {
        OUTCOME_TRY(visit(item2.second, visitor));
      }
    } else {
      for (auto &pair : boost::get<Node::Leaf>(item)) {
        OUTCOME_TRY(visitor(pair.first, pair.second));
      }
    }
    return outcome::success();
  }
}  // namespace fc::storage::hamt
