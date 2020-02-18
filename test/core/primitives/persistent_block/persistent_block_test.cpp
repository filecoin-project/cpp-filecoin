/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/persistent_block/persistent_block.hpp"

#include <gtest/gtest.h>
#include "common/buffer.hpp"
#include "primitives/cid/cid_of_cbor.hpp"
#include "testutil/outcome.hpp"

using namespace fc::primitives;

struct PersistentBlockTest : public ::testing::Test {
  using PersistentBlock = blockchain::block::PersistentBlock;
  void SetUp() override {
    EXPECT_OUTCOME_TRUE(c1, cid::getCidOfCbor(std::string("cid1")));
    EXPECT_OUTCOME_TRUE(c2, cid::getCidOfCbor(std::string("cid2")));
    cid1 = c1;
    cid2 = c2;
    content1.put("value1");
    content2.put("value2");
  }

  fc::CID cid1;
  fc::CID cid2;

  fc::common::Buffer content1;
  fc::common::Buffer content2;
};

/**
 * @brief simple test, ensuring that created block contains data
 * it was created with
 */
TEST_F(PersistentBlockTest, CreatePersistentBlockSuccess) {
  PersistentBlock pb(cid1, content1);
  ASSERT_EQ(pb.getCID(), cid1);
  ASSERT_EQ(pb.getRawBytes(), content1);
}
