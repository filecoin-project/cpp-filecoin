/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipfs/merkledag/impl/service_impl.hpp"

#include <boost/assert.hpp>
#include "storage/ipfs/merkledag/impl/node_impl.hpp"

namespace fc::storage::ipfs::merkledag {
  ServiceImpl::ServiceImpl(std::shared_ptr<BlockService> service)
      : block_service_{std::move(service)} {
    BOOST_ASSERT_MSG(block_service_ != nullptr,
                     "MerkleDAG service: Block service not connected");
  }

  outcome::result<void> ServiceImpl::addNode(std::shared_ptr<const Node> node) {
    return block_service_->addBlock(*node);
  }

  outcome::result<std::shared_ptr<Node>> ServiceImpl::getNode(
      const CID &cid) const {
    OUTCOME_TRY(content, block_service_->getBlockContent(cid));
    return NodeImpl::createFromRawBytes(content);
  }

  outcome::result<void> ServiceImpl::removeNode(const CID &cid) {
    return block_service_->removeBlock(cid);
  }

  outcome::result<std::shared_ptr<Leave>> ServiceImpl::fetchGraph(
      const CID &cid) const {
    OUTCOME_TRY(node, getNode(cid));
    auto root_leave = std::make_shared<LeaveImpl>(node->content());
    auto result = buildGraph(root_leave, node->getLinks(), false, 0, 0);
    if (result.has_error()) return result.error();
    return root_leave;
  }

  outcome::result<std::shared_ptr<Leave>> ServiceImpl::fetchGraphOnDepth(
      const CID &cid, uint64_t depth) const {
    OUTCOME_TRY(node, getNode(cid));
    auto leave = std::make_shared<LeaveImpl>(node->content());
    auto result = buildGraph(leave, node->getLinks(), true, depth, 0);
    if (result.has_error()) return result.error();
    return leave;
  }

  outcome::result<void> ServiceImpl::buildGraph(
      const std::shared_ptr<LeaveImpl> &root,
      const std::vector<std::reference_wrapper<const Link>> &links,
      bool depth_limit,
      const size_t max_depth,
      size_t current_depth) const {
    if (depth_limit && current_depth == max_depth) {
      return outcome::success();
    }
    for (const auto &link : links) {
      auto request = getNode(link.get().getCID());
      if (request.has_error()) return ServiceError::UNRESOLVED_LINK;
      std::shared_ptr<Node> node = request.value();
      auto child_leave = std::make_shared<LeaveImpl>(node->content());
      auto build_result = buildGraph(child_leave,
                                     node->getLinks(),
                                     depth_limit,
                                     max_depth,
                                     ++current_depth);
      if (build_result.has_error()) {
        return build_result;
      }
      auto insert_result =
          root->insertSubLeave(link.get().getName(), std::move(*child_leave));
      if (insert_result.has_error()) {
        return insert_result;
      }
    }
    return outcome::success();
  }
}  // namespace fc::storage::ipfs::merkledag

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::ipfs::merkledag, ServiceError, e) {
  using fc::storage::ipfs::merkledag::ServiceError;
  switch (e) {
    case (ServiceError::UNRESOLVED_LINK):
      return "MerkleDAG service: broken link";
  }
  return "MerkleDAG Node: unknown error";
}
