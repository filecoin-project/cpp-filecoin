/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/go/math.hpp"

#include <gtest/gtest.h>

namespace fc {
  using fc::primitives::BigInt;

  /// `/` is boost::multiprecision behavior with round-to-zero.
  /// `bigdiv` is compatible `big.Div` in go with round-floor.
  TEST(BigInt, Divide) {
    ASSERT_EQ(BigInt{4} / BigInt{3}, BigInt{1});
    ASSERT_EQ(bigdiv(4, 3), BigInt{1});
    ASSERT_EQ(BigInt{-4} / BigInt{3}, BigInt{-1});
    ASSERT_EQ(bigdiv(-4, 3), BigInt{-2});
  }

  TEST(BigInt, DivideByZero) {
    BigInt a{8};
    BigInt b{0};
    ASSERT_THROW(BigInt{a / b}, std::exception);
    ASSERT_THROW(bigdiv(a, b), std::exception);
  }

  TEST(BigInt, DivideMod) {
    BigInt a{-4};
    BigInt b{3};
    ASSERT_EQ(a % b, BigInt{-1});
    ASSERT_EQ(bigmod(a, b), BigInt{2});
  }

}  // namespace fc
