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
    case AmtError::kRootBitsWrong:
      return "AmtError::kRootBitsWrong";
    case AmtError::kNodeBitsWrong:
      return "AmtError::kNodeBitsWrong";
  }
  return "Unknown error";
}

namespace fc::storage::amt {
  CBOR2_ENCODE(Node) {
    std::vector<uint8_t> bits;
    bits.resize(v.bits_bytes);
    auto bit{[&](auto i) { bits[i / 8] |= 1 << (i % 8); }};
    auto l_links = s.list();
    auto l_values = s.list();
    visit_in_place(
        v.items,
        [&bit, &l_links](const Node::Links &links) {
          for (auto &item : links) {
            bit(item.first);
            if (which<Node::Ptr>(item.second)) {
              outcome::raise(AmtError::kExpectedCID);
            }
            l_links << boost::get<CID>(item.second);
          }
        },
        [&bit, &l_values](const Node::Values &values) {
          for (auto &item : values) {
            bit(item.first);
            l_values << l_values.wrap(item.second, 1);
          }
        });
    return s << (s.list() << bits << l_links << l_values);
  }
  CBOR2_DECODE(Node) {
    auto l_node = s.list();
    auto bits{l_node.get<std::vector<uint8_t>>()};
    std::vector<size_t> indices;
    for (auto i = 0u; i < bits.size(); ++i) {
      for (auto j = 0u; j < 8; ++j) {
        if (bits[i] & (1 << j)) {
          indices.push_back(8 * i + j);
        }
      }
    }

    const auto n_links = l_node.listLength();
    auto l_links = l_node.list();
    const auto n_values = l_node.listLength();
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
        links[indices[i]] = l_links.get<CID>();
      }
      v.items = links;
    } else {
      if (n_values != indices.size()) {
        outcome::raise(AmtError::kDecodeWrong);
      }
      Node::Values values;
      for (auto i = 0u; i < n_values; ++i) {
        values[indices[i]] = Value{l_values.raw()};
      }
      v.items = values;
    }
    v.bits_bytes = bits.size();
    return s;
  }

  CBOR2_DECODE(Root) {
    const auto n{s.listLength()};
    auto l{s.list()};
    v.bits.reset();
    if (n == 4) {
      v.bits = l.get<uint64_t>();
    }
    l >> v.height >> v.count >> v.node;
    return s;
  }
  CBOR2_ENCODE(Root) {
    auto l{s.list()};
    if (v.bits) {
      l << *v.bits;
    }
    l << v.height << v.count << v.node;
    return s << l;
  }

  Amt::Amt(std::shared_ptr<ipfs::IpfsDatastore> store, size_t bits)
      : ipld(std::move(store)), root_{}, bits_{bits} {
    assert(bits);
  }

  Amt::Amt(std::shared_ptr<ipfs::IpfsDatastore> store,
           const CID &root,
           size_t bits)
      : ipld(std::move(store)), root_(root), bits_{bits} {
    assert(bits);
  }

  outcome::result<uint64_t> Amt::count() const {
    OUTCOME_TRY(loadRoot());
    return boost::get<Root>(root_).count;
  }

  outcome::result<void> Amt::set(uint64_t key, gsl::span<const uint8_t> value) {
    if (key > kMaxIndex) {
      return AmtError::kIndexTooBig;
    }
    OUTCOME_TRY(loadRoot());
    auto &root = boost::get<Root>(root_);
    while (key >= maxAt(root.height)) {
      if (!visit_in_place(root.node.items,
                          [](auto &xs) { return xs.empty(); })) {
        root.node = {
            Node::Links{{0, std::make_shared<Node>(std::move(root.node))}},
            bitsBytes(),
        };
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
    if (key > kMaxIndex) {
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
    if (key > kMaxIndex) {
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
      if (links.size() != 1 || links.find(0) == links.end()) {
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
    lazyCreateRoot();
    if (which<Root>(root_)) {
      auto &root = boost::get<Root>(root_);
      OUTCOME_TRY(flush(root.node));
      OUTCOME_TRY(root_cid, fc::setCbor(ipld, root));
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
          OUTCOME_TRY(cid, fc::setCbor(ipld, child));
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
    lazyCreateRoot();
    if (which<CID>(root_)) {
      OUTCOME_TRY(root, fc::getCbor<Root>(ipld, boost::get<CID>(root_)));
      root_ = root;
      if (v3() ? root.bits != bits() : root.bits.has_value()) {
        return AmtError::kRootBitsWrong;
      }
      if (root.node.bits_bytes != bitsBytes()) {
        return AmtError::kRootBitsWrong;
      }
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
        node->bits_bytes = bitsBytes();
        links[index] = node;
        return node;
      }
      return AmtError::kNotFound;
    }
    auto &link = it->second;
    if (which<CID>(link)) {
      OUTCOME_TRY(node, fc::getCbor<Node>(ipld, boost::get<CID>(link)));
      if (node.bits_bytes != bitsBytes()) {
        return AmtError::kRootBitsWrong;
      }
      link = std::make_shared<Node>(std::move(node));
    }
    return boost::get<Node::Ptr>(link);
  }

  uint64_t Amt::bits() const {
    return v3() ? bits_ : kDefaultBits;
  }

  uint64_t Amt::bitsBytes() const {
    if (bits() <= 3) {
      return 1;
    }
    return 1 << (bits() - 3);
  }

  uint64_t Amt::maskAt(uint64_t height) const {
    return 1 << (bits() * height);
  }

  uint64_t Amt::maxAt(uint64_t height) const {
    return maskAt(height + 1);
  }

  void Amt::lazyCreateRoot() const {
    if (which<std::monostate>(root_)) {
      Root root;
      if (v3()) {
        root.bits = bits();
      }
      root.node.bits_bytes = bitsBytes();
      root_ = std::move(root);
    }
  }

  bool Amt::v3() const {
    return ipld->actor_version >= vm::actor::ActorVersion::kVersion3;
  }
}  // namespace fc::storage::amt
