/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adt/uvarint_key.hpp"

#include <gtest/gtest.h>

using fc::adt::UvarintKey;
using namespace std::string_literals;

TEST(StoragePowerActorState, EncodeChainEpoch) {
  EXPECT_EQ(UvarintKey::encode(0), "\x00"s);
  EXPECT_EQ(UvarintKey::encode(1), "\x01"s);
  EXPECT_EQ(UvarintKey::encode(2), "\x02"s);
  EXPECT_EQ(UvarintKey::encode(100), "\x64"s);
  EXPECT_EQ(UvarintKey::encode(130), "\x82\x1"s);
}
