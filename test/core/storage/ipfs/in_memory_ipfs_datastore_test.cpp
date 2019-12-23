/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/buffer_generator.hpp"
#include "testutil/cid_generator.hpp"
#include "testutil/outcome.hpp"

using fc::storage::ipfs::InMemoryDatastore;
using fc::storage::ipfs::IpfsDatastore;
using fc::storage::ipfs::IpfsDatastoreError;
using testutils::BufferGenerator;
using testutils::CidGenerator;

class InMemoryIpfsDatastoreTest : public ::testing::Test {
 public:
  std::shared_ptr<IpfsDatastore> datastore;
  static constexpr size_t kDefaultBufferSize = 32u;

  void SetUp() override {
    datastore = std::make_shared<InMemoryDatastore>();
  }

  BufferGenerator buffer_generator;
  CidGenerator cid_generator;
};

/**
 * @given opened datastore, cid and a value
 * @when put cid with value into datastore @and then get value from datastore by
 * cid
 * @then all operation succeeded, obtained value is equal to original value
 */
TEST_F(InMemoryIpfsDatastoreTest, ContainsExistingTrueSuccess) {
  auto value = buffer_generator.makeRandomBuffer(kDefaultBufferSize);
  auto cid = cid_generator.makeRandomCid();
  EXPECT_OUTCOME_TRUE_1(datastore->set(cid, value));
  EXPECT_OUTCOME_TRUE(res, datastore->contains(cid));
  ASSERT_TRUE(res);
}

/**
 * @given opened datastore, 2 different instances of CID and a value
 * @when put cid1 with value into datastore and check if datastore contains cid2
 * @then all operations succeed and datastore doesn't contains cid2
 */
TEST_F(InMemoryIpfsDatastoreTest, ContainsNotExistingFalseSuccess) {
  auto cid1 = cid_generator.makeRandomCid();
  auto cid2 = cid_generator.makeRandomCid();
  auto value = buffer_generator.makeRandomBuffer(kDefaultBufferSize);
  EXPECT_OUTCOME_TRUE_1(datastore->set(cid1, value));
  EXPECT_OUTCOME_TRUE(res, datastore->contains(cid2));
  ASSERT_FALSE(res);
}

/**
 * @given opened datastore, CID instance and a value
 * @when put cid with value into datastore @and then get value by cid
 * @then all operations succeed
 */
TEST_F(InMemoryIpfsDatastoreTest, GetExistingSuccess) {
  auto cid = cid_generator.makeRandomCid();
  auto value = buffer_generator.makeRandomBuffer(kDefaultBufferSize);
  EXPECT_OUTCOME_TRUE_1(datastore->set(cid, value));
  EXPECT_OUTCOME_TRUE(v, datastore->get(cid));
  ASSERT_EQ(v, value);
}

/**
 * @given opened datastore, 2 different CID instances and a value
 * @when put cid1 with value into datastore @and then get value by cid2
 * @then put operation succeeds, get operation fails
 */
TEST_F(InMemoryIpfsDatastoreTest, GetNotExistingFailure) {
  auto cid1 = cid_generator.makeRandomCid();
  auto cid2 = cid_generator.makeRandomCid();
  auto value = buffer_generator.makeRandomBuffer(kDefaultBufferSize);
  EXPECT_OUTCOME_TRUE_1(datastore->set(cid1, value));
  EXPECT_OUTCOME_ERROR(IpfsDatastoreError::NOT_FOUND, datastore->get(cid2));
}

/**
 * @given opened datastore, CID instance and a value
 * @when put cid with value into datastore @and remove cid from datastore
 * @then all operations succeed and datastore doesn't contain cid anymore
 */
TEST_F(InMemoryIpfsDatastoreTest, RemoveSuccess) {
  auto cid = cid_generator.makeRandomCid();
  auto value = buffer_generator.makeRandomBuffer(kDefaultBufferSize);
  EXPECT_OUTCOME_TRUE_1(datastore->set(cid, value));
  EXPECT_OUTCOME_TRUE_1(datastore->remove(cid));
  EXPECT_OUTCOME_TRUE(res, datastore->contains(cid));
  ASSERT_FALSE(res);
}

/**
 * @given opened datastore, 2 CID instances and a value
 * @when put cid1 with value into datastore @and remove cid2 from datastore
 * @then all operations succeed and datastore still contains cid1
 */
TEST_F(InMemoryIpfsDatastoreTest, RemoveNotExistingSuccess) {
  auto cid1 = cid_generator.makeRandomCid();
  auto cid2 = cid_generator.makeRandomCid();
  auto value = buffer_generator.makeRandomBuffer(kDefaultBufferSize);
  EXPECT_OUTCOME_TRUE_1(datastore->set(cid1, value));
  EXPECT_OUTCOME_TRUE_1(datastore->remove(cid2));
  EXPECT_OUTCOME_TRUE(res, datastore->contains(cid1));
  ASSERT_TRUE(res);
}
