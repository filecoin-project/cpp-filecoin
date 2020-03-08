/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/storage/chain/datastore_key.hpp"

#include <gtest/gtest.h>

#include <iostream>

using namespace fc::storage;

struct DatastoreKeyCreateTest
    : public ::testing::TestWithParam<std::pair<std::string, std::string>> {
  void SetUp() override {}
};

namespace {
  auto make(std::string_view s, std::string_view v) {
    return std::pair<std::string, std::string>(s, v);
  }
}  // namespace

/**
 * @given set of pairs where lhs is source string and rhs is key value
 * @when create key from string
 * @then key value is equal to predefined one
 */
TEST_P(DatastoreKeyCreateTest, CreateFromStringSuccess) {
  auto &[source, value] = GetParam();

  auto key = DatastoreKey::makeFromString(source);
  ASSERT_EQ(key.value, value);
  std::cout << key.value << std::endl;
}

INSTANTIATE_TEST_CASE_P(DatastoreKeyCreateTestCases,
                        DatastoreKeyCreateTest,
                        ::testing::Values(make("", "/"),
                                          make("/", "/"),
                                          make("a", "/a"),
                                          make("abcd", "/abcd"),
                                          make("a/b", "/a/b"),
                                          make(" ", "/ "),
                                          make("a//b/./c", "/a/b/c"),
                                          make("a//b/../c", "/a/c"),
                                          make("a/", "/a"),
                                          make("/.", "/")));
