/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/amt/amt.hpp"

#include "common/which.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::amt, AmtError, e) {
  using fc::storage::amt::AmtError;
  switch (e) {
    case AmtError::kExpectedCID:
      return "Expected CID";
    case AmtError::kDecodeWrong:
      return "Decode wrong";
    case AmtError::kIndexTooBig:
      return "Index too big";
    case AmtError::kNotFound:
      return "Not found";
  }
  return "Unknown error";
}

namespace fc::storage::amt {
  auto pow(uint64_t base, uint64_t exponent) {
    uint64_t result{1};
    while (exponent != 0) {
      if (exponent % 2 != 0) {
        result *= base;
        --exponent;
      }
      exponent /= 2;
      base *= base;
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
      : ipld(std::move(store)), root_(Root{}) {}

  Amt::Amt(std::shared_ptr<ipfs::IpfsDatastore> store, const CID &root)
      : ipld(std::move(store)), root_(root) {}

  outcome::result<uint64_t> Amt::count() const {
    OUTCOME_TRY(loadRoot());
    return boost::get<Root>(root_).count;
  }

  outcome::result<void> Amt::set(uint64_t key, gsl::span<const uint8_t> value) {
    if (key >= kMaxIndex) {
      return AmtError::kIndexTooBig;
    }
    OUTCOME_TRY(loadRoot());
    auto &root = boost::get<Root>(root_);
    while (key >= maxAt(root.height)) {
      if (!visit_in_place(root.node.items,
                          [](auto &xs) { return xs.empty(); })) {
        root.node = {
            Node::Links{{0, std::make_shared<Node>(std::move(root.node))}}};
      }
      ++root.height;
    }
    OUTCOME_TRY(add, set(root.node, root.height, key, value));
    if (add) {
      ++root.count;
    }
    return outcome::success();
  }

  outcome::result<Value> Amt::get(uint64_t key) const {
    if (key >= kMaxIndex) {
      return AmtError::kIndexTooBig;
    }
    OUTCOME_TRY(loadRoot());
    auto &root = boost::get<Root>(root_);
    if (key >= maxAt(root.height)) {
      return AmtError::kNotFound;
    }
    std::reference_wrapper<Node> node = root.node;
    for (auto height = root.height; height != 0; --height) {
      auto mask = maskAt(height);
      OUTCOME_TRY(child, loadLink(node, key / mask, false));
      key %= mask;
      node = *child;
    }
    auto &values = boost::get<Node::Values>(node.get().items);
    auto it = values.find(key);
    if (it == values.end()) {
      return AmtError::kNotFound;
    }
    return it->second;
  }

  outcome::result<void> Amt::remove(uint64_t key) {
    if (key >= kMaxIndex) {
      return AmtError::kIndexTooBig;
    }
    OUTCOME_TRY(loadRoot());
    auto &root = boost::get<Root>(root_);
    if (key >= maxAt(root.height)) {
      return AmtError::kNotFound;
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
      OUTCOME_TRY(root_cid, ipld->setCbor(root));
      root_ = root_cid;
    }
    return cid();
  }

  const CID &Amt::cid() const {
    return boost::get<CID>(root_);
  }

  outcome::result<void> Amt::visit(const Visitor &visitor) const {
    OUTCOME_TRY(loadRoot());
    auto &root = boost::get<Root>(root_);
    return visit(root.node, root.height, 0, visitor);
  }

  outcome::result<bool> Amt::set(Node &node,
                                 uint64_t height,
                                 uint64_t key,
                                 gsl::span<const uint8_t> value) {
    if (height == 0) {
      auto &values = boost::get<Node::Values>(node.items);
      if (values.insert(std::make_pair(key, value)).second) {
        return true;
      }
      values.at(key) = Value(value);
      return false;
    }
    auto mask = maskAt(height);
    OUTCOME_TRY(child, loadLink(node, key / mask, true));
    return set(*child, height - 1, key % mask, value);
  }

  outcome::result<bool> Amt::remove(Node &node, uint64_t height, uint64_t key) {
    if (height == 0) {
      auto &values = boost::get<Node::Values>(node.items);
      if (values.erase(key) == 0) {
        return AmtError::kNotFound;
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
        [](const Node::Links &links) { return links.empty(); });
    if (empty) {
      boost::get<Node::Links>(node.items).erase(index);
    }
    return outcome::success();
  }

  outcome::result<bool> Amt::contains(uint64_t key) const {
    auto res = get(key);
    if (res) {
      return true;
    }
    if (res.error() == AmtError::kNotFound) {
      return false;
    }
    return res.error();
  }

  outcome::result<void> Amt::flush(Node &node) {
    if (which<Node::Links>(node.items)) {
      auto &links = boost::get<Node::Links>(node.items);
      for (auto &pair : links) {
        if (which<Node::Ptr>(pair.second)) {
          auto &child = *boost::get<Node::Ptr>(pair.second);
          OUTCOME_TRY(flush(child));
          OUTCOME_TRY(cid, ipld->setCbor(child));
          pair.second = cid;
        }
      }
    }
    return outcome::success();
  }

  outcome::result<void> Amt::visit(Node &node,
                                   uint64_t height,
                                   uint64_t offset,
                                   const Visitor &visitor) const {
    if (height == 0) {
      for (auto &it : boost::get<Node::Values>(node.items)) {
        OUTCOME_TRY(visitor(offset + it.first, it.second));
      }
      return outcome::success();
    }
    auto mask = maskAt(height);
    for (auto &it : boost::get<Node::Links>(node.items)) {
      OUTCOME_TRY(loadLink(node, it.first, false));
      OUTCOME_TRY(visit(*boost::get<Node::Ptr>(it.second),
                        height - 1,
                        offset + it.first * mask,
                        visitor));
    }
    return outcome::success();
  }

  outcome::result<void> Amt::loadRoot() const {
    if (which<CID>(root_)) {
      OUTCOME_TRY(root, ipld->getCbor<Root>(boost::get<CID>(root_)));
      root_ = root;
    }
    return outcome::success();
  }

  outcome::result<Node::Ptr> Amt::loadLink(Node &parent,
                                           uint64_t index,
                                           bool create) const {
    if (which<Node::Values>(parent.items)
        && boost::get<Node::Values>(parent.items).empty()) {
      parent.items = Node::Links{};
    }
    auto &links = boost::get<Node::Links>(parent.items);
    auto it = links.find(index);
    if (it == links.end()) {
      if (create) {
        auto node = std::make_shared<Node>();
        links[index] = node;
        return node;
      }
      return AmtError::kNotFound;
    }
    auto &link = it->second;
    if (which<CID>(link)) {
      OUTCOME_TRY(node, ipld->getCbor<Node>(boost::get<CID>(link)));
      link = std::make_shared<Node>(std::move(node));
    }
    return boost::get<Node::Ptr>(link);
  }
}  // namespace fc::storage::amt
