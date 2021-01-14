/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/rle_bitset/runs_utils.hpp"

#include <gtest/gtest.h>

namespace fc::primitives {

  /**
   * @given Two runs vectors
   * @when make or operation
   * @then result vector is symbiosis of two vectors
   */
  TEST(RunsUtils, OrSuccess) {
    std::vector<uint64_t> lhs = {0, 127, 100, 127};
    std::vector<uint64_t> rhs = {10, 117, 227, 100};
    std::vector<uint64_t> result = {0, 127, 100, 227};

    ASSERT_EQ(runsOr(lhs, rhs), result);
  }

  /**
   * @given One runs vector
   * @when make or operation with empty one
   * @then result vector is same with initial vector
   */
  TEST(RunsUtils, OrWithEmpty) {
    std::vector<uint64_t> lhs = {0, 127, 100, 127};
    std::vector<uint64_t> rhs = {0};

    ASSERT_EQ(runsOr(lhs, rhs), lhs);
    ASSERT_EQ(runsOr(rhs, lhs), lhs);
  }

  /**
   * @given Two runs vectors
   * @when make and operation
   * @then result vector is common sectors of two vectors
   */
  TEST(RunsUtils, AndSuccess) {
    std::vector<uint64_t> lhs = {0, 127, 100, 127};
    std::vector<uint64_t> rhs = {10, 117, 227, 100};
    std::vector<uint64_t> result = {10, 117, 227};  // reduced to less

    ASSERT_EQ(runsAnd(lhs, rhs), result);
  }

  /**
   * @given One runs vector
   * @when make and operation with empty one
   * @then result vector is empty
   */
  TEST(RunsUtils, AndWithEmpty) {
    std::vector<uint64_t> lhs = {0, 127, 100, 127};
    std::vector<uint64_t> rhs = {0};

    ASSERT_EQ(runsAnd(lhs, rhs), rhs);
    ASSERT_EQ(runsAnd(rhs, lhs), rhs);
  }

  /**
   * @given Two runs vectors
   * @when make substract operation
   * @then result vector is first minus second sectors
   */
  TEST(RunsUtils, AndWithSubstract) {
    std::vector<uint64_t> lhs = {0, 1024};
    std::vector<uint64_t> rhs = {127, 127, 100, 100};
    std::vector<uint64_t> result = {0, 127, 127, 100, 100, 570};
    ASSERT_EQ(runsAnd(lhs, rhs, true), result);
  }

  /**
   * @given One runs vector
   * @when make substract operation with itself
   * @then result vector is offset of all size
   */
  TEST(RunsUtils, SubstractItself) {
    std::vector<uint64_t> lhs = {0, 1024};
    std::vector<uint64_t> result = {1024};  // offset only
    ASSERT_EQ(runsAnd(lhs, lhs, true), result);
  }

  /**
   * @given Two runs vectors. second is tail of the first one
   * @when make substract operation
   * @then result vector is small first vector
   */
  TEST(RunsUtils, SubstractTail) {
    std::vector<uint64_t> lhs = {0, 1024};
    std::vector<uint64_t> rhs = {924, 100};
    std::vector<uint64_t> result = {0, 924, 100};  // offset only
    ASSERT_EQ(runsAnd(lhs, rhs, true), result);
  }
}  // namespace fc::primitives
