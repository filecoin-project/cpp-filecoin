/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/storage/chain/datastore_key.hpp"

#include <gtest/gtest.h>
#include "filecoin/codec/cbor/cbor.hpp"
#include "testutil/outcome.hpp"

using namespace fc::storage;

using fc::codec::cbor::encode;
using fc::codec::cbor::decode;

struct DatastoreKetCborTest : public ::testing::Test {
  void SetUp() override {
    key1 = DatastoreKey::makeFromString("/a/b/c");
    key2 = DatastoreKey::makeFromString("a/b/d");

    for (auto &s : data) {
      keys.push_back(DatastoreKey::makeFromString(s));
    }
  }

  DatastoreKey key1;
  DatastoreKey key2;

  std::vector<std::string> data {"", "/a", "b", "/a/b", "a/b/c", "a/b/d"};
  std::vector<DatastoreKey> keys;
};

/** @brief ensure that different keys correspond to different encoded values */
TEST_F(DatastoreKetCborTest, InjectivenessSuccess) {
  EXPECT_OUTCOME_TRUE(enc1, encode(key1));
  EXPECT_OUTCOME_TRUE(enc2, encode(key2));
  ASSERT_NE(enc1, enc2);
}

/** @brief ensure that encode and then decode makes original key */
TEST_F(DatastoreKetCborTest, EncodeDecodeSuccess) {
  auto check_reversability = [&](const DatastoreKey &key) {
    EXPECT_OUTCOME_TRUE(enc, encode(key));
    EXPECT_OUTCOME_TRUE(key_r, decode<DatastoreKey>(enc));
    ASSERT_EQ(key, key_r);
  };

  for (auto &k : keys) {
    check_reversability(k);
  }
}
