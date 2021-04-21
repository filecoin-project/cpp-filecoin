/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/quantize.hpp"

#include <gtest/gtest.h>

namespace fc::vm::actor::builtin::types::miner {
  TEST(QuantizeTest, NoQuantization) {
    EXPECT_EQ(0, kNoQuantization.quantizeUp(0));
    EXPECT_EQ(1, kNoQuantization.quantizeUp(1));
    EXPECT_EQ(2, kNoQuantization.quantizeUp(2));
    EXPECT_EQ(123456789, kNoQuantization.quantizeUp(123456789));
  }

  TEST(QuantizeTest, ZeroOffset) {
    EXPECT_EQ(50, QuantSpec(10, 0).quantizeUp(42));
    EXPECT_EQ(16000, QuantSpec(100, 0).quantizeUp(16000));
    EXPECT_EQ(0, QuantSpec(10, 0).quantizeUp(-5));
    EXPECT_EQ(-50, QuantSpec(10, 0).quantizeUp(-50));
    EXPECT_EQ(-50, QuantSpec(10, 0).quantizeUp(-53));
  }

  TEST(QuantizeTest, NonZeroOffset) {
    EXPECT_EQ(6, QuantSpec(5, 1).quantizeUp(4));
    EXPECT_EQ(1, QuantSpec(5, 1).quantizeUp(0));
    EXPECT_EQ(-4, QuantSpec(5, 1).quantizeUp(-6));
    EXPECT_EQ(4, QuantSpec(10, 4).quantizeUp(2));
  }

  TEST(QuantizeTest, BigOffset) {
    EXPECT_EQ(13, QuantSpec(5, 28).quantizeUp(9));
    EXPECT_EQ(10000, QuantSpec(100, 2000000).quantizeUp(10000));
  }

}  // namespace fc::vm::actor::builtin::types::miner
