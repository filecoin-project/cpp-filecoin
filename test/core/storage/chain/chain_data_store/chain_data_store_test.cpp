/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/chain/impl/chain_data_store_impl.hpp"

#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include <gtest/gtest.h>
#include "testutil/outcome.hpp"

using namespace fc::storage::blockchain;
using namespace fc::storage;

struct ChainDataStoreTest : public ::testing::Test {
  void SetUp() override {
    auto ipfs_store = std::make_shared<ipfs::InMemoryDatastore>();
    store_ = std::make_shared<ChainDataStoreImpl>(std::move(ipfs_store));
    key1 = DatastoreKey::makeFromString("key1");
    key2 = DatastoreKey::makeFromString("key2");
    value1 = "value1";

  }

  std::shared_ptr<ChainDataStoreImpl> store_;
  DatastoreKey key1;
  DatastoreKey key2;
  std::string value1;
};

/**
 * @given chain data storage, a key and a value
 * @when add value to storage
 * @then storage contains added value
 * @and doesn't contain another value, which wasn't added
 */
TEST_F(ChainDataStoreTest, AddValueSuccess) {
  EXPECT_OUTCOME_TRUE_1(store_->set(key1, value1));
  EXPECT_OUTCOME_TRUE(contains1, store_->contains(key1));
  ASSERT_TRUE(contains1);
  EXPECT_OUTCOME_TRUE(contains2, store_->contains(key2));
  ASSERT_FALSE(contains2);
}

/**
 * @given chain data storage, a key and a value
 * @when add value to storage
 * @then storage contains added value
 * @when remove specified key
 * @then storage now doesn't contain specified key
 */
TEST_F(ChainDataStoreTest, RemoveValueSuccess) {
  EXPECT_OUTCOME_TRUE_1(store_->set(key1, value1));
  EXPECT_OUTCOME_TRUE(contains1, store_->contains(key1));
  ASSERT_TRUE(contains1);
  EXPECT_OUTCOME_TRUE_1(store_->remove(key1));
  EXPECT_OUTCOME_TRUE(contains2, store_->contains(key1));
  ASSERT_FALSE(contains2);
}

/**
 * @given chain data storage, a key and a value
 * @when add value to storage
 * @then storage contains added value
 * @when get value by key
 * @then obtained value is equal to specfied value
 */
TEST_F(ChainDataStoreTest, GetValueSuccess) {
  EXPECT_OUTCOME_TRUE_1(store_->set(key1, value1));
  EXPECT_OUTCOME_TRUE(contains1, store_->contains(key1));
  ASSERT_TRUE(contains1);
  EXPECT_OUTCOME_TRUE(value,store_->get(key1));
  ASSERT_EQ(value, value1);
}
