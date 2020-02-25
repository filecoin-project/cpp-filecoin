/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/chain/datastore_key.hpp"

#include <gtest/gtest.h>

using namespace fc::storage;

struct DatastoreKeyCompareTest : public ::testing::Test {
  void SetUp() override {
    for (auto &s : data) {
      keys.push_back(DatastoreKey::makeFromString(s));
    }

    key = DatastoreKey::makeFromString("abcd");
  }

  std::vector<std::string> data{"", "/a", "b", "/a/b", "a/b/c", "a/b/d"};
  std::vector<DatastoreKey> keys;
  DatastoreKey key;
};

/** @brief ensure reflexiveness */
TEST_F(DatastoreKeyCompareTest, ReflexivenessSuccess) {
  for (auto &k : keys) {
    ASSERT_EQ(k, k);
  }
}

/** @check that different keys don't match */
TEST_F(DatastoreKeyCompareTest, NotEqualSuccess) {
  for (auto &k : keys) {
    ASSERT_TRUE(!(k == key));
  }
}

/** @brief ensure that less works correctly on keys */
TEST_F(DatastoreKeyCompareTest, LessSuccess) {
  auto make = [&](std::string_view s) -> DatastoreKey {
    return DatastoreKey::makeFromString(s);
  };

  ASSERT_TRUE(make("/a/b/c") < make("/a/b/c/d"));
  ASSERT_TRUE(make("/a/b") < make("/a/b/c/d"));
  ASSERT_TRUE(make("/a") < make("/a/b/c/d"));
  ASSERT_TRUE(make("/a/a/c") < make("/a/b/c"));
  ASSERT_TRUE(make("/a/a/d") < make("/a/b/c"));
  ASSERT_TRUE(make("/a/b/c/d/e/f/g/h") < make("/b"));
  ASSERT_TRUE(make("/") < make("/a"));

  ASSERT_FALSE(make("/a/b/c/d") < make("/a/b/c"));
  ASSERT_FALSE(make("/a/b/c/d") < make("/a/b"));
  ASSERT_FALSE(make("/a/b/c/d") < make("/a"));
  ASSERT_FALSE(make("/a/b/c") < make("/a/a/c"));
  ASSERT_FALSE(make("/a/b/c") < make("/a/a/d"));
  ASSERT_FALSE(make("/b") < make("/a/b/c/d/e/f/g/h"));
  ASSERT_FALSE(make("/a") < make("/"));
}
