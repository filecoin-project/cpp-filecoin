/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adt/uvarint_key.hpp"

#include <gtest/gtest.h>

using ChainEpochKeyer = fc::adt::UvarintKeyer;
using namespace std::string_literals;

TEST(StoragePowerActorState, EncodeChainEpoch) {
  EXPECT_EQ(ChainEpochKeyer::encode(0), "\x00"s);
  EXPECT_EQ(ChainEpochKeyer::encode(1), "\x01"s);
  EXPECT_EQ(ChainEpochKeyer::encode(2), "\x02"s);
  EXPECT_EQ(ChainEpochKeyer::encode(100), "\x64"s);
  EXPECT_EQ(ChainEpochKeyer::encode(130), "\x82\x1"s);
}
