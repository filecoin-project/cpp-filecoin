/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/piece/piece.hpp"

#include <gtest/gtest.h>

/**
 * @given some sizes
 * @when using paddedSize for transform to valid padded size
 * @then getting unpadded sizes for sectors
 */
TEST(PaddedSizeTest, PaddedSize) {
  using fc::primitives::piece::paddedSize;
  ASSERT_EQ(1040384, paddedSize(1000000));

  ASSERT_EQ(1016, paddedSize(548));
  ASSERT_EQ(1016, paddedSize(1015));
  ASSERT_EQ(1016, paddedSize(1016));
  ASSERT_EQ(2032, paddedSize(1017));

  ASSERT_EQ(2032, paddedSize(1024));
  ASSERT_EQ(4064, paddedSize(2048));
}
