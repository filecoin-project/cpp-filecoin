/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/amt/amt.hpp"

#include "common/which.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::amt, AmtError, e) {
  using fc::storage::amt::AmtError;
  switch (e) {
    case AmtError::EXPECTED_CID:
      return "Expected CID";
    case AmtError::DECODE_WRONG:
      return "Decode wrong";
    case AmtError::INDEX_TOO_BIG:
      return "Index too big";
    case AmtError::NOT_FOUND:
      return "Not found";
  }
  return "Unknown error";
}

namespace fc::storage::amt {
  auto pow(uint64_t base, uint64_t exponent) {
    uint64_t result{1};
    if (exponent) {
      --exponent;
      result *= base;
    }
    while (exponent) {
      if (exponent % 2 == 0) {
        exponent /= 2;
        result *= result;
      } else {
        --exponent;
        result *= base;
      }
    }
    return result;
  }

  auto maskAt(uint64_t height) {
    return pow(kWidth, height);
  }

  auto maxAt(uint64_t height) {
    return maskAt(height + 1);
  }

  Amt::Amt(std::shared_ptr<ipfs::IpfsDatastore> store)
      : store_(std::move(store)), root_(Root{}) {}

  Amt::Amt(std::shared_ptr<ipfs::IpfsDatastore> store, const CID &root)
      : store_(std::move(store)), root_(root) {}

  outcome::result<uint64_t> Amt::count() {
    OUTCOME_TRY(loadRoot());
    return boost::get<Root>(root_).count;
  }

  outcome::result<void> Amt::set(uint64_t key, gsl::span<const uint8_t> value) {
    if (key >= kMaxIndex) {
      return AmtError::INDEX_TOO_BIG;
    }
    OUTCOME_TRY(loadRoot());
    auto &root = boost::get<Root>(root_);
    while (key >= maxAt(root.height)) {
      root.node = {true, Node::Links{{0, std::make_shared<Node>(std::move(root.node))}}};
      ++root.height;
    }
    OUTCOME_TRY(add, set(root.node, root.height, key, value));
    if (add) {
      ++root.count;
    }
    return outcome::success();
  }

  outcome::result<Value> Amt::get(uint64_t key) {
    if (key >= kMaxIndex) {
      return AmtError::INDEX_TOO_BIG;
    }
    OUTCOME_TRY(loadRoot());
    auto &root = boost::get<Root>(root_);
    if (key >= maxAt(root.height)) {
      return AmtError::NOT_FOUND;
    }
    std::reference_wrapper<Node> node = root.node;
    for (auto height = root.height; height; --height) {
      auto mask = maskAt(height);
      OUTCOME_TRY(child, loadLink(node, key / mask, false));
      key %= mask;
      node = *child;
    }
    auto &values = boost::get<Node::Values>(node.get().items);
    auto it = values.find(key);
    if (it == values.end()) {
      return AmtError::NOT_FOUND;
    }
    return it->second;
  }

  outcome::result<void> Amt::remove(uint64_t key) {
    if (key >= kMaxIndex) {
      return AmtError::INDEX_TOO_BIG;
    }
    OUTCOME_TRY(loadRoot());
    auto &root = boost::get<Root>(root_);
    if (key >= maxAt(root.height)) {
      return AmtError::NOT_FOUND;
    }
    OUTCOME_TRY(remove(root.node, root.height, key));
    --root.count;
    while (root.height > 0) {
      auto &links = boost::get<Node::Links>(root.node.items);
      if (links.size() != 1 && links.find(0) == links.end()) {
        break;
      }
      OUTCOME_TRY(child, loadLink(root.node, 0, false));
      auto node = std::move(*child);
      root.node = std::move(node);
      --root.height;
    }
    return outcome::success();
  }

  outcome::result<CID> Amt::flush() {
    if (which<Root>(root_)) {
      auto &root = boost::get<Root>(root_);
      OUTCOME_TRY(flush(root.node));
      OUTCOME_TRY(cid, store_->setCbor(root));
      root_ = cid;
    }
    return boost::get<CID>(root_);
  }

  outcome::result<void> Amt::visit(const Visitor &visitor) {
    OUTCOME_TRY(loadRoot());
    auto &root = boost::get<Root>(root_);
    return visit(root.node, root.height, 0, visitor);
  }

  outcome::result<bool> Amt::set(Node &node, uint64_t height, uint64_t key, gsl::span<const uint8_t> value) {
    node.has_bits = true;
    if (height == 0) {
      return boost::get<Node::Values>(node.items).insert(std::make_pair(key, value)).second;
    }
    auto mask = maskAt(height);
    OUTCOME_TRY(child, loadLink(node, key / mask, true));
    return set(*child, height - 1, key % mask, value);
  }

  outcome::result<bool> Amt::remove(Node &node, uint64_t height, uint64_t key) {
    if (height == 0) {
      auto &values = boost::get<Node::Values>(node.items);
      if (values.erase(key) == 0) {
        return AmtError::NOT_FOUND;
      }
      return outcome::success();
    }
    auto mask = maskAt(height);
    auto index = key / mask;
    OUTCOME_TRY(child, loadLink(node, index, false));
    OUTCOME_TRY(remove(*child, height - 1, key % mask));
    // github.com/filecoin-project/go-amt-ipld/v2 behavior
    auto empty = visit_in_place(
      child->items,
      [](const Node::Values &values) { return values.empty(); },
      [](const Node::Links &links) { return links.empty(); }
      );
    if (empty) {
      boost::get<Node::Links>(node.items).erase(index);
    }
    return outcome::success();
  }

  outcome::result<void> Amt::flush(Node &node) {
    if (which<Node::Links>(node.items)) {
      auto &links = boost::get<Node::Links>(node.items);
      for (auto &pair : links) {
        if (which<Node::Ptr>(pair.second)) {
          auto &child = *boost::get<Node::Ptr>(pair.second);
          OUTCOME_TRY(flush(child));
          OUTCOME_TRY(cid, store_->setCbor(child));
          pair.second = cid;
        }
      }
    }
    return outcome::success();
  }

  outcome::result<void> Amt::visit(Node &node, uint64_t height, uint64_t offset, const Visitor &visitor) {
    if (height == 0) {
      for (auto &it : boost::get<Node::Values>(node.items)) {
        OUTCOME_TRY(visitor(offset + it.first, it.second));
      }
      return outcome::success();
    }
    auto mask = maskAt(height);
    for (auto &it : boost::get<Node::Links>(node.items)) {
      OUTCOME_TRY(loadLink(node, it.first, false));
      OUTCOME_TRY(visit(*boost::get<Node::Ptr>(it.second), height - 1, offset + it.first * mask, visitor));
    }
    return outcome::success();
  }

  outcome::result<void> Amt::loadRoot() {
    if (which<CID>(root_)) {
      OUTCOME_TRY(root, store_->getCbor<Root>(boost::get<CID>(root_)));
      root_ = root;
    }
    return outcome::success();
  }

  outcome::result<Node::Ptr> Amt::loadLink(Node &parent, uint64_t index, bool create) {
    if (which<Node::Values>(parent.items) && boost::get<Node::Values>(parent.items).empty()) {
      parent.items = Node::Links{};
    }
    auto &links = boost::get<Node::Links>(parent.items);
    auto it = links.find(index);
    if (it == links.end()) {
      if (create) {
        auto node = std::make_shared<Node>();
        links[index] = node;
        return node;
      } else {
        return AmtError::NOT_FOUND;
      }
    }
    auto &link = it->second;
    if (which<CID>(link)) {
      OUTCOME_TRY(node, store_->getCbor<Node>(boost::get<CID>(link)));
      link = std::make_shared<Node>(std::move(node));
    }
    return boost::get<Node::Ptr>(link);
  }
}  // namespace fc::storage::amt
