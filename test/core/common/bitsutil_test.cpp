/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/bitsutil.hpp"
#include <gtest/gtest.h>

namespace fc::common {

  /**
   * Check trailing zeroes counting
   */
  TEST(Bitsutil, countTrailingZeros) {
    EXPECT_EQ(64, countTrailingZeros(0));
    uint64_t x = 1;
    for (int zeroes = 0; zeroes < 64; ++zeroes) {
      EXPECT_EQ(zeroes, countTrailingZeros(x));
      x = x << 1;
    }
  }

}  // namespace fc::common
