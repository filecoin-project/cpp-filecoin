/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipfs/impl/ipfs_block_service.hpp"

#include <memory>

#include <gtest/gtest.h>
#include "common/outcome.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "storage/ipld/ipld_block.hpp"
#include "testutil/outcome.hpp"

using fc::CID;
using fc::common::Buffer;
using fc::common::getCidOf;
using fc::storage::ipfs::InMemoryDatastore;
using fc::storage::ipld::IPLDBlock;
using fc::storage::ipfs::IpfsBlockService;

/**
 * @brief Implementation of Block interface for testing purposes
 * This interface can be used by any data structure (like Node from
 * MerkleDAG service) and there are no single universal implementation
 */
struct BlockTestImpl : IPLDBlock {
  Buffer content; /**< Raw data */
  CID cid;                /**< Block identifier */

  /**
   * @brief Construct Block
   * @param data - raw content
   */
  BlockTestImpl(std::vector<uint8_t> data)
      : content{data}, cid{getCidOf(content).value()} {}

  /**
   * @brief Get content identifier
   * @return Block's CID
   */
  const CID &getCID() const override {
    return this->cid;
  }

  /**
   * @brief Get block's content
   * @details Type of the return value depends on Block's interface
   * implementation: it can be raw bytes, or CBOR/Protobuf serialized value
   * @return Block's data
   */
  const Buffer &getRawBytes() const override {
    return this->content;
  }
};

/**
 * @class Test fixture for BlockService
 */
class BlockServiceTest : public ::testing::Test {
 public:
  /**
   * @brief Initialize BlockService
   */
  BlockServiceTest() : block_service_(std::make_shared<InMemoryDatastore>()) {}

 protected:
  IpfsBlockService block_service_; /**< Testing target */

  BlockTestImpl sample_block_{
      {4, 8, 15, 16, 23, 42}}; /**< Sample block with pre-defined data */
};

/**
 * @given Sample block with pre-defined data
 * @when Adding, checking existence and retrieving block back from BlockStorage
 * @then Add block @and check its existence @and retrieve block back from
 * BlockStorage
 */
TEST_F(BlockServiceTest, StoreBlockSuccess) {
  EXPECT_OUTCOME_TRUE_1(block_service_.set(sample_block_.getCID(), sample_block_.getRawBytes()))
  EXPECT_OUTCOME_TRUE(contains, block_service_.contains(sample_block_.getCID()))
  ASSERT_TRUE(contains);
  EXPECT_OUTCOME_TRUE(block_content,
                      block_service_.get(sample_block_.getCID()))
  ASSERT_EQ(block_content, sample_block_.getRawBytes());
}

/**
 * @given CID of the block, which doesn't exist in the BlockService
 * @when Trying to check block existence in the BlockService
 * @then Operation must be completed successfully with result "not exists"
 */
TEST_F(BlockServiceTest, CheckExistenceSuccess) {
  EXPECT_OUTCOME_TRUE(contains, block_service_.contains(sample_block_.getCID()))
  ASSERT_FALSE(contains);
}

/**
 * @given Sample block with pre-defined data
 * @when Trying to remove an existence block
 * @then Operation must be completed successfully
 */
TEST_F(BlockServiceTest, RemoveBlockSuccess) {
  EXPECT_OUTCOME_TRUE_1(block_service_.set(sample_block_.getCID(), sample_block_.getRawBytes()))
  const auto &cid = sample_block_.getCID();
  EXPECT_OUTCOME_TRUE(block_status, block_service_.contains(cid))
  ASSERT_TRUE(block_status);
  EXPECT_OUTCOME_TRUE_1(block_service_.remove(cid))
  EXPECT_OUTCOME_TRUE(removed_status, block_service_.contains(cid))
  ASSERT_FALSE(removed_status);
}

/**
 * @given CID of the block, which doesn't exist in the BlockService
 * @when Try to get nonexistent block from BlockService
 * @then Attempt fails
 */
TEST_F(BlockServiceTest, GetInvalidCidFailure) {
  const auto &cid = sample_block_.getCID();
  EXPECT_OUTCOME_FALSE(result, block_service_.get(cid))
  std::ignore = result;
}
