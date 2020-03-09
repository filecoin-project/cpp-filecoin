/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/storage/ipfs/merkledag/impl/leaf_impl.hpp"

namespace fc::storage::ipfs::merkledag {
  LeafImpl::LeafImpl(common::Buffer data)
      : content_{std::move(data)} {}

  const common::Buffer &LeafImpl::content() const {
    return content_;
  }

  size_t LeafImpl::count() const {
    return children_.size();
  }

  outcome::result<std::reference_wrapper<const Leaf>> LeafImpl::subLeaf(
      std::string_view name) const {
    if (auto iter = children_.find(name); iter != children_.end()) {
      return iter->second;
    }
    return LeafError::LEAF_NOT_FOUND;
  }

  std::vector<std::string_view> LeafImpl::getSubLeafNames() const {
    std::vector<std::string_view> names;
    for (const auto &child : children_) {
      names.push_back(child.first);
    }
    return names;
  }

  outcome::result<void> LeafImpl::insertSubLeaf(std::string name,
                                                 LeafImpl children) {
    auto result = children_.emplace(std::move(name), std::move(children));
    if (result.second) {
      return outcome::success();
    }
    return LeafError::DUPLICATE_LEAF;
  }
}  // namespace fc::storage::ipfs::merkledag

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::ipfs::merkledag, LeafError, e) {
  using fc::storage::ipfs::merkledag::LeafError;
  switch (e) {
    case (LeafError::LEAF_NOT_FOUND):
      return "MerkleDAG leaf: children leaf not found";
    case (LeafError::DUPLICATE_LEAF):
      return "MerkleDAG leaf: duplicate leaf name";
  }
  return "MerkleDAG leaf: unknown error";
}
