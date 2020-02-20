/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/rle_bitset/rle_bitset.hpp"

#include <gtest/gtest.h>
#include "testutil/cbor.hpp"

/**
 * @given rle bitset and its serialized representation from go
 * @when encode @and decode the rle bitset
 * @then decoded version matches the original @and encoded matches the go ones
 */
TEST(RleBitsetTest, RleBitsetCbor) {
  using fc::primitives::RleBitset;
  expectEncodeAndReencode(RleBitset{2, 7}, "43504a01"_unhex);
}
