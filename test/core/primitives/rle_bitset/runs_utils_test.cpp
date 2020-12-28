/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/rle_bitset/runs_utils.hpp"

#include <gtest/gtest.h>

TEST(RunsUtils, OrSuccess) {
  using namespace fc::primitives;
  std::vector<uint64_t> lhs = {0, 127, 100, 127};
  std::vector<uint64_t> rhs = {10, 117, 227, 100};
  std::vector<uint64_t> result = {0, 127, 100, 227};

  ASSERT_EQ(runsOr(lhs, rhs), result);
}

TEST(RunsUtils, OrWithEmpty) {
  using namespace fc::primitives;
  std::vector<uint64_t> lhs = {0, 127, 100, 127};
  std::vector<uint64_t> rhs = {0};

  ASSERT_EQ(runsOr(lhs, rhs), lhs);
  ASSERT_EQ(runsOr(rhs, lhs), lhs);
}

TEST(RunsUtils, AndSuccess) {
  using namespace fc::primitives;
  std::vector<uint64_t> lhs = {0, 127, 100, 127};
  std::vector<uint64_t> rhs = {10, 117, 227, 100};
  std::vector<uint64_t> result = {10, 117, 227};  // reduced to less

  ASSERT_EQ(runsAnd(lhs, rhs), result);
}

TEST(RunsUtils, AndWithEmpty) {
  using namespace fc::primitives;
  std::vector<uint64_t> lhs = {0, 127, 100, 127};
  std::vector<uint64_t> rhs = {0};

  ASSERT_EQ(runsAnd(lhs, rhs), rhs);
  ASSERT_EQ(runsAnd(rhs, lhs), rhs);
}

TEST(RunsUtils, AndWithSubstract) {
  using namespace fc::primitives;
  std::vector<uint64_t> lhs = {0, 1024};
  std::vector<uint64_t> rhs = {127, 127, 100, 100};
  std::vector<uint64_t> result = {0, 127, 127, 100, 100, 570};
  ASSERT_EQ(runsAnd(lhs, rhs, true), result);
}

TEST(RunsUtils, SubstractItself) {
  using namespace fc::primitives;
  std::vector<uint64_t> lhs = {0, 1024};
  std::vector<uint64_t> result = {1024}; // offset only
  ASSERT_EQ(runsAnd(lhs, lhs, true), result);
}

TEST(RunsUtils, SubstractTail) {
    using namespace fc::primitives;
    std::vector<uint64_t> lhs = {0, 1024};
    std::vector<uint64_t> rhs = {924, 100};
    std::vector<uint64_t> result = {0, 924, 100}; // offset only
    ASSERT_EQ(runsAnd(lhs, rhs, true), result);
}
