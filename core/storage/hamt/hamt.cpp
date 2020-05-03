/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/hamt/hamt.hpp"

#include "common/which.hpp"
#include "crypto/murmur/murmur.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::hamt, HamtError, e) {
  using fc::storage::hamt::HamtError;
  switch (e) {
    case HamtError::EXPECTED_CID:
      return "Expected CID";
    case HamtError::NOT_FOUND:
      return "Not found";
    case HamtError::MAX_DEPTH:
      return "Max depth exceeded";
  }
  return "Unknown error";
}

namespace fc::storage::hamt {
  using fc::common::which;

  auto consumeIndex(gsl::span<const size_t> indices) {
    return indices.subspan(1);
  }

  Hamt::Hamt(std::shared_ptr<ipfs::IpfsDatastore> store, size_t bit_width)
      : store_{std::move(store)},
        root_{std::make_shared<Node>()},
        bit_width_{bit_width} {}

  Hamt::Hamt(std::shared_ptr<ipfs::IpfsDatastore> store,
             Node::Ptr root,
             size_t bit_width)
      : store_{std::move(store)},
        root_{std::move(root)},
        bit_width_{bit_width} {}

  Hamt::Hamt(std::shared_ptr<ipfs::IpfsDatastore> store,
             const CID &root,
             size_t bit_width)
      : store_{std::move(store)}, root_{root}, bit_width_{bit_width} {}

  outcome::result<void> Hamt::set(const std::string &key,
                                  gsl::span<const uint8_t> value) {
    OUTCOME_TRY(loadItem(root_));
    return set(*boost::get<Node::Ptr>(root_), keyToIndices(key), key, value);
  }

  outcome::result<Value> Hamt::get(const std::string &key) {
    OUTCOME_TRY(loadItem(root_));
    auto node = boost::get<Node::Ptr>(root_);
    for (auto index : keyToIndices(key)) {
      auto it = node->items.find(index);
      if (it == node->items.end()) {
        return HamtError::NOT_FOUND;
      }
      auto &item = it->second;
      OUTCOME_TRY(loadItem(item));
      if (which<Node::Ptr>(item)) {
        node = boost::get<Node::Ptr>(item);
      } else {
        auto &leaf = boost::get<Node::Leaf>(item);
        if (leaf.find(key) == leaf.end()) {
          return HamtError::NOT_FOUND;
        }
        return leaf[key];
      }
    }
    return HamtError::MAX_DEPTH;
  }

  outcome::result<void> Hamt::remove(const std::string &key) {
    OUTCOME_TRY(loadItem(root_));
    return remove(*boost::get<Node::Ptr>(root_), keyToIndices(key), key);
  }

  outcome::result<bool> Hamt::contains(const std::string &key) {
    auto res = get(key);
    if (!res) {
      if (res.error() == HamtError::NOT_FOUND) return false;
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

  std::vector<size_t> Hamt::keyToIndices(const std::string &key, int n) const {
    std::vector<uint8_t> key_bytes(key.begin(), key.end());
    auto hash = crypto::murmur::hash(key_bytes);
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
                                  const std::string &key,
                                  gsl::span<const uint8_t> value) {
    if (indices.empty()) {
      return HamtError::MAX_DEPTH;
    }
    auto index = indices[0];
    auto it = node.items.find(index);
    if (it == node.items.end()) {
      Node::Leaf leaf;
      leaf.emplace(key, std::vector<uint8_t>(value.begin(), value.end()));
      node.items[index] = leaf;
      return outcome::success();
    }
    auto &item = it->second;
    OUTCOME_TRY(loadItem(item));
    if (which<Node::Ptr>(item)) {
      return set(
          *boost::get<Node::Ptr>(item), consumeIndex(indices), key, value);
    }
    auto &leaf = boost::get<Node::Leaf>(item);
    if (leaf.find(key) != leaf.end() || leaf.size() < kLeafMax) {
      leaf[key] = Value(value);
    } else {
      auto child = std::make_shared<Node>();
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
                                     const std::string &key) {
    if (indices.empty()) {
      return HamtError::MAX_DEPTH;
    }
    auto index = indices[0];
    auto it = node.items.find(index);
    if (it == node.items.end()) {
      return HamtError::NOT_FOUND;
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
        return HamtError::NOT_FOUND;
      }
      if (leaf.size() == 1) {
        node.items.erase(index);
      } else {
        leaf.erase(key);
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
      OUTCOME_TRY(cid, store_->setCbor(node));
      item = cid;
    }
    return outcome::success();
  }

  outcome::result<void> Hamt::loadItem(Node::Item &item) const {
    if (which<CID>(item)) {
      OUTCOME_TRY(child, store_->getCbor<Node>(boost::get<CID>(item)));
      item = std::make_shared<Node>(std::move(child));
    }
    return outcome::success();
  }

  outcome::result<void> Hamt::visit(const Visitor &visitor) {
    return visit(root_, visitor);
  }

  outcome::result<void> Hamt::visit(Node::Item &item, const Visitor &visitor) {
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
