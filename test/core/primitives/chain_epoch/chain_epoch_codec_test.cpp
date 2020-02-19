/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/chain_epoch/chain_epoch_codec.hpp"

#include <gtest/gtest.h>

using fc::primitives::ChainEpoch;
using fc::primitives::chain_epoch::encodeToByteString;
using namespace std::string_literals;

TEST(StoragePowerActorState, EncodeChainEpoch) {
  EXPECT_EQ(encodeToByteString({0}), "\x00"s);
  EXPECT_EQ(encodeToByteString({1}), "\x01"s);
  EXPECT_EQ(encodeToByteString({2}), "\x02"s);
  EXPECT_EQ(encodeToByteString({100}), "\x64"s);
  EXPECT_EQ(encodeToByteString({130}), "\x82\x1"s);
}
