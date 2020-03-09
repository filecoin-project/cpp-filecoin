/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/primitives/big_int.hpp"

#include <gtest/gtest.h>

using fc::primitives::BigInt;

/**
 * Test arithmetis over BigInt types
 */

TEST(BigInt, Equality) {
  BigInt a{2};
  BigInt b{2};
  ASSERT_TRUE(a == b);
  ASSERT_TRUE(a == a);
}

TEST(BigInt, SelfMultiply) {
  BigInt a{2};
  a *= BigInt{3};
  ASSERT_EQ(a, BigInt{6});
}

TEST(BigInt, Multiply) {
  BigInt a{2};
  BigInt b{3};
  ASSERT_EQ(a * b, BigInt{6});
}

TEST(BigInt, SelfDivide) {
  BigInt a{8};
  a /= BigInt{2};
  ASSERT_EQ(a, BigInt{4});
}

TEST(BigInt, Divide) {
  BigInt a{8};
  BigInt b{2};
  ASSERT_EQ(a / b, BigInt{4});
}

TEST(BigInt, DivideByZero) {
  BigInt a{8};
  BigInt b{0};
  ASSERT_THROW(BigInt{a / b}, std::exception);
}
