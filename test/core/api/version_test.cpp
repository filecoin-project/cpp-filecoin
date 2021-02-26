/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "api/version.hpp"

namespace fc::api {
  /**
   * Test API version compatibility with Lotus.
   */
  TEST(ApiVersionTest, Version) {
    EXPECT_EQ(0x000C00, makeApiVersion(0, 12, 0));
    EXPECT_EQ(0x010000, makeApiVersion(1, 0, 0));
    EXPECT_EQ(0x0000FF, makeApiVersion(0, 0, 255));
    EXPECT_EQ(0x010203, makeApiVersion(1, 2, 3));
  }
}  // namespace fc::api
