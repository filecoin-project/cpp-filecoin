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
  ChainEpoch v_0{0};
  ChainEpoch v_1{1};
  ChainEpoch v_2{2};
  ChainEpoch v_100{100};
  EXPECT_EQ(encodeToByteString(v_0), "\x00"s);
  EXPECT_EQ(encodeToByteString(v_1), "\x01"s);
  EXPECT_EQ(encodeToByteString(v_2), "\x02"s);
  EXPECT_EQ(encodeToByteString(v_100), "\x64"s);
}
