/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/murmur/murmur.hpp"

#include <gtest/gtest.h>
#include "testutil/literals.hpp"

/**
 * @given Bytes
 * @when Hash
 * @then Equals to pregenerated hash
 */
TEST(Murmur, GoCompatibility) {
  EXPECT_EQ(fc::crypto::murmur::hash("3031323334353637383961626364656630"_unhex), "EB24AE8785A5C075"_unhex);
}
