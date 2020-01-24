/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipfs/merkledag/impl/leave_impl.hpp"

namespace fc::storage::ipfs::merkledag {
  LeaveImpl::LeaveImpl(common::Buffer data)
      : content_{std::move(data)} {}

  const common::Buffer &LeaveImpl::content() const {
    return content_;
  }

  size_t LeaveImpl::count() const {
    return children_.size();
  }

  outcome::result<std::reference_wrapper<const Leave>> LeaveImpl::subLeave(
      std::string_view name) const {
    if (auto iter = children_.find(name); iter != children_.end()) {
      return iter->second;
    }
    return LeaveError::LEAVE_NOT_FOUND;
  }

  std::vector<std::string_view> LeaveImpl::getSubLeaveNames() const {
    std::vector<std::string_view> names;
    for (const auto &child : children_) {
      names.push_back(child.first);
    }
    return names;
  }

  outcome::result<void> LeaveImpl::insertSubLeave(std::string name,
                                                  LeaveImpl children) {
    auto result = children_.emplace(std::move(name), std::move(children));
    if (result.second) {
      return outcome::success();
    }
    return LeaveError::DUPLICATE_LEAVE;
  }
}  // namespace fc::storage::ipfs::merkledag

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::ipfs::merkledag, LeaveError, e) {
  using fc::storage::ipfs::merkledag::LeaveError;
  switch (e) {
    case (LeaveError::LEAVE_NOT_FOUND):
      return "MerkleDAG leave: children leave not found";
    case (LeaveError::DUPLICATE_LEAVE):
      return "MerkleDAG leave: duplicate leave name";
  }
  return "MerkleDAG leave: unknown error";
}
