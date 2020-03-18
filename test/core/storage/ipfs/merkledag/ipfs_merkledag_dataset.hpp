/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CORE_STORAGE_IPFS_MERKLEDAG_NODE_LIBRARY
#define CORE_STORAGE_IPFS_MERKLEDAG_NODE_LIBRARY

#include "storage/ipld/impl/ipld_node_impl.hpp"

namespace dataset {
  using fc::storage::ipld::IPLDNode;
  using fc::storage::ipld::IPLDNodeImpl;

  /**
   * @brief Add child link for root node
   * @param to - root node
   * @param from - child node
   * @return operation result
   */
  fc::outcome::result<void> link(const std::shared_ptr<IPLDNode> &to,
                                 const std::shared_ptr<const IPLDNode> &from) {
    OUTCOME_TRY(from_id, from->getCID().toString());
    EXPECT_OUTCOME_TRUE_1(to->addChild(from_id, from));
    return fc::outcome::success();
  }

  /**
   * @brief Generate node suite type A
   * @return Null node
   */
  std::vector<std::shared_ptr<IPLDNode>> getTypeA() {
    return {IPLDNodeImpl::createFromString("")};
  }

  /**
   * @brief Generate node suite type B
   * @return Node without child
   */
  std::vector<std::shared_ptr<IPLDNode>> getTypeB() {
    return {IPLDNodeImpl::createFromString("leve1_node1")};
  }

  /**
   * @brief Generate node suite type C
   * @return Node with one child
   *                    []
   *                    |
   *              [leve1_node1]
   */
  std::vector<std::shared_ptr<IPLDNode>> getTypeC() {
    auto root = IPLDNodeImpl::createFromString("");
    auto child_1 = IPLDNodeImpl::createFromString("leve1_node1");
    EXPECT_OUTCOME_TRUE_1(link(root, child_1));
    return {root, child_1};
  }

  /**
   * @brief Generate node suite type D
   * @return Node with several childs child
   *                 [leve1_node1]
   *               /      |       \
   * [leve2_node1]  [leve2_node2]  [leve2_node3]
   *
   */
  std::vector<std::shared_ptr<IPLDNode>> getTypeD() {
    auto root = IPLDNodeImpl::createFromString("leve1_node1");
    auto child_1 = IPLDNodeImpl::createFromString("leve2_node1");
    EXPECT_OUTCOME_TRUE_1(link(root, child_1));
    auto child_2 = IPLDNodeImpl::createFromString("leve2_node2");
    EXPECT_OUTCOME_TRUE_1(link(root, child_2));
    auto child_3 = IPLDNodeImpl::createFromString("leve2_node3");
    EXPECT_OUTCOME_TRUE_1(link(root, child_3));
    return {root, child_1, child_2, child_3};
  }

  /**
   * @brief Generate node suite type E
   * @return Node with two child "branches" and node, which is child of two
   * different parents
   *                               [] ---------------
   *                              / \               |
   *                 [leve1_node1]   [leve1_node2]  |
   *               /      |       \                 |
   * [leve2_node1]  [leve2_node2]  [leve2_node3]-----
   */
  std::vector<std::shared_ptr<IPLDNode>> getTypeE() {
    auto root = IPLDNodeImpl::createFromString("");
    auto suite_D = getTypeD();
    EXPECT_OUTCOME_TRUE_1(link(root, suite_D.front()));
    auto child_1 = IPLDNodeImpl::createFromString("leve1_node2");
    EXPECT_OUTCOME_TRUE_1(link(root, child_1));
    EXPECT_OUTCOME_TRUE_1(link(root, suite_D.back()));
    std::vector<std::shared_ptr<IPLDNode>> suite_E{root, child_1};
    suite_E.insert(suite_E.end(), suite_D.begin(), suite_D.end());
    return suite_E;
  }
}  // namespace dataset

#endif
