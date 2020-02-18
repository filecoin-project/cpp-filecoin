/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipfs/merkledag/impl/node_impl.hpp"

#include <gtest/gtest.h>
#include <libp2p/multi/content_identifier_codec.hpp>
#include <storage/ipfs/merkledag/impl/merkledag_service_impl.hpp>
#include <testutil/outcome.hpp>
#include "core/storage/ipfs/merkledag/ipfs_merkledag_dataset.hpp"
#include "storage/ipfs/impl/ipfs_block_service.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"

using namespace fc::storage::ipfs;
using namespace fc::storage::ipfs::merkledag;
using libp2p::multi::ContentIdentifierCodec;

/**
 * @struct Test case dataset
 */
struct DataSample {
  // Various-linked MerkleDAG nodes [root, node_1, node_2 ... ]
  std::vector<std::shared_ptr<Node>> nodes;

  // Base58-encoded CID of the root node from the Lotus implementation
  std::string sample_cid;

  // Graph structure, defined like: {} is node, [] is content, -> is children
  std::string graph_structure;
};

/**
 * @struct Test fixture for MerkleDAG service
 */
struct CommonFeaturesTest : public testing::TestWithParam<DataSample> {
  // MerkleDAG service
  std::shared_ptr<MerkleDagService> merkledag_service_{nullptr};

  // Sample data for current test case
  DataSample data;

  /**
   * @brief Prepare test suite
   */
  void SetUp() override {
    std::shared_ptr<IpfsDatastore> datastore{new InMemoryDatastore{}};
    std::shared_ptr<IpfsDatastore> blockservice{new IpfsBlockService{datastore}};
    merkledag_service_ = std::make_shared<MerkleDagServiceImpl>(blockservice);
    data = GetParam();
    EXPECT_OUTCOME_TRUE_1(this->saveToBlockService(data.nodes));
  }

  /**
   * @brief Save nodes to the Block Service
   * @param nodes - data to save
   * @return operation result
   */
  fc::outcome::result<void> saveToBlockService(
      const std::vector<std::shared_ptr<Node>> &nodes) {
    for (const auto &node : nodes) {
      EXPECT_OUTCOME_TRUE_1(merkledag_service_->addNode(node));
    }
    return fc::outcome::success();
  }

  /**
   * @brief Get CID string representation node
   * @param node - target for retrieving id
   * @return Base58-encoded CID v0
   */
  static std::string cidToString(const std::shared_ptr<const Node> &node) {
    std::string value{};
    auto result = ContentIdentifierCodec::toString(node->getCID());
    if (!result.has_error()) {
      value = result.value();
    }
    return value;
  }

  /**
   * @brief Generate graph structure
   *        Node without content:  {[]}
   *        Node without children: {[content]}
   *        Node with children:    {[content]->{[children]}]
   * @note  This serialized graph structure uses only for tests purposes
   * @param leaf - root node
   * @return operation result
   */
  static std::string getGraphStructure(const Leaf &leaf) {
    std::string content{"{["};
    std::string data{leaf.content().begin(), leaf.content().end()};
    content.append(data);
    content.push_back(']');
    std::vector<std::string_view> leaves = leaf.getSubLeafNames();
    if (!leaves.empty()) {
      content.append("->");
      for (const auto &leaf_id : leaves) {
        EXPECT_OUTCOME_TRUE(sub_leaf, leaf.subLeaf(leaf_id))
        content.append(getGraphStructure(sub_leaf));
        content.push_back(',');
      }
      content.pop_back();
    }
    content.push_back('}');
    return content;
  }
};

/**
 * @given Pre-generated nodes and reference CIDs
 * @when Attempt to get stored node by CID
 * @then MerkleDAG service returns requested node
 */
TEST_P(CommonFeaturesTest, GetNodeSuccess) {
  for (const auto &node : data.nodes) {
    EXPECT_OUTCOME_TRUE(received_node,
                        merkledag_service_->getNode(node->getCID()))
    ASSERT_EQ(cidToString(node), cidToString(received_node));
  }
}

/**
 * @given Pre-generated nodes and reference CIDs
 * @when Calculating CID of the root node
 * @then Calculated and reference CIDs must be equal
 */
TEST_P(CommonFeaturesTest, CheckCidAlgorithmSuccess) {
  ASSERT_EQ(data.sample_cid, cidToString(data.nodes.front()));
}

/**
 * @given Pre-generated nodes sets with children links
 * @when Removing and restoring children links
 * @then Attempt to retrieve removed link must be failed,
 *       attempt to restore removed link must be successful,
 *       attempt to restrieve restored link must be successful,
 *       CIDs of the node before and after all operations must be equal
 */
TEST_P(CommonFeaturesTest, LinkOperationsConsistency) {
  for (const auto &node : data.nodes) {
    std::vector<std::reference_wrapper<const Link>> links = node->getLinks();
    std::string primary_cid = cidToString(node);
    for (const auto &link : links) {
      LinkImpl link_impl{
          link.get().getCID(), link.get().getName(), link.get().getSize()};
      EXPECT_OUTCOME_TRUE(received_link, node->getLink(link.get().getName()));
      std::ignore = received_link;
      std::string link_name = link.get().getName();
      node->removeLink(link_name);
      EXPECT_OUTCOME_FALSE(null_result, node->getLink(link_name));
      std::ignore = null_result;
      node->addLink(link_impl);
      EXPECT_OUTCOME_TRUE(restored_link, node->getLink(link_name));
      std::ignore = restored_link;
    }
    std::string secondary_cid = cidToString(node);
    ASSERT_EQ(primary_cid, secondary_cid);
  }
}

/**
 * @given Pre-generated node
 * @when Removing node from MerkleDAG service
 * @then Attempt to get removed node must be failed
 */
TEST_P(CommonFeaturesTest, GetInvalidNodeFail) {
  const auto &cid = data.nodes.front()->getCID();
  EXPECT_OUTCOME_TRUE_1(merkledag_service_->removeNode(cid));
  EXPECT_OUTCOME_FALSE_1(merkledag_service_->getNode(cid));
};

/**
 * @given Pre-generated node
 * @when Retrieving non-existent link from node
 * @then Attempt to get non-existent link must be failed
 */
TEST_P(CommonFeaturesTest, GetInvalidLinkFail) {
  const auto &node = data.nodes.front();
  std::string invalid_name = "non_existent_link_name";
  EXPECT_OUTCOME_FALSE_1(node->getLink(invalid_name))
}

/**
 * @given Pre-generated nodes structure and reference serialized structure
 * @when Fetching node and all children recursively
 * @then Serialized node structure and reference value must be equal
 */
TEST_P(CommonFeaturesTest, FetchGraphSuccess) {
  const auto &root_cid = data.nodes.front()->getCID();
  EXPECT_OUTCOME_TRUE(root_leaf, merkledag_service_->fetchGraph(root_cid))
  std::string fetched_structure = getGraphStructure(*root_leaf);
  ASSERT_EQ(fetched_structure, data.graph_structure);
}

/**
 * @given Pre-generated nodes structure
 * @when Selecting nodes from DAG service
 * @then All operations must be successful and
 *       selected nodes count must be count of root node children + 1 (self)
 */
TEST_P(CommonFeaturesTest, GraphSyncSelect) {
  const size_t nodes_count = data.nodes.front()->getLinks().size() + 1;
  EXPECT_OUTCOME_TRUE(
      root_cid, ContentIdentifierCodec::encode(data.nodes.front()->getCID()));
  std::vector<std::shared_ptr<const Node>> selected_nodes;
  std::function<bool(std::shared_ptr<const Node>)> handler =
      [&selected_nodes](std::shared_ptr<const Node> node) -> bool {
    selected_nodes.emplace_back(std::move(node));
    return true;
  };
  EXPECT_OUTCOME_TRUE(selected_count,
                      merkledag_service_->select(
                          root_cid, gsl::span<const uint8_t>{}, handler));
  ASSERT_EQ(selected_nodes.size(), selected_count);
  ASSERT_EQ(nodes_count, selected_count);
}

/**
 * Pre-generated nodes, CIDs and serialized graph structures
 * Reference CIDs was generated by https://github.com/ipfs/go-merkledag
 */
INSTANTIATE_TEST_CASE_P(
    PredefinedSamples,
    CommonFeaturesTest,
    testing::Values(
        DataSample{dataset::getTypeA(),
                   "QmdfTbBqBPQ7VNxZEYEj14VmRuZBkqFbiwReogJgS1zR1n",
                   "{[]}"},
        DataSample{dataset::getTypeB(),
                   "QmTq5KSpqFrzJTQ7LCDCr7GmKZrWW46pp2DSrW3WibgFV6",
                   "{[leve1_node1]}"},
        DataSample{dataset::getTypeC(),
                   "Qmaybnje7u6r2suDoUehBqxJU8dQ3MKwrUJ5Bi7qt68rDg",
                   "{[]->{[leve1_node1]}}"},
        DataSample{
            dataset::getTypeD(),
            "QmWocGTL2xjWpckYEAHCAK3MES4MjGudmz8KAiYP9m7wEs",
            "{[leve1_node1]->{[leve2_node2]},{[leve2_node1]},{[leve2_node3]}}"},
        DataSample{dataset::getTypeE(),
                   "QmaKbJN4obBb7D1Ko3Ar5xrsaon4HbFeiCMNAW9g94ufmo",
                   "{[]->{[leve1_node1]->{[leve2_node2]},{[leve2_node1]},{["
                   "leve2_node3]}},{[leve2_node3]},{[leve1_node2]}}"}));
