/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clock/impl/chain_epoch_clock_impl.hpp"

#include <gtest/gtest.h>

#include "testutil/outcome.hpp"

using fc::clock::ChainEpoch;
using fc::clock::ChainEpochClockImpl;
using fc::clock::kEpochDuration;
using fc::clock::UnixTime;

static UnixTime kGenesisTime{1};
static UnixTime kSec1{1};
static ChainEpoch kEpoch(3);
static UnixTime kSecEpoch{kEpoch * kEpochDuration};

class ChainEpochClockTest : public ::testing::Test {
 public:
  ChainEpochClockImpl epoch_clock{kGenesisTime};
};

/**
 * @given genesis time
 * @when construct ChainEpochClock and get genesisTime
 * @then equals to original
 */
TEST_F(ChainEpochClockTest, GenesisTime) {
  EXPECT_EQ(epoch_clock.genesisTime(), kGenesisTime);
}

/**
 * @given time just before genesis
 * @when epochAtTime
 * @then error
 */
TEST_F(ChainEpochClockTest, BeforeGenesis) {
  EXPECT_OUTCOME_FALSE_1(epoch_clock.epochAtTime(kGenesisTime - kSec1));
}

/**
 * @given time at genesis
 * @when epochAtTime
 * @then epoch 0
 */
TEST_F(ChainEpochClockTest, AtGenesis) {
  EXPECT_OUTCOME_EQ(epoch_clock.epochAtTime(kGenesisTime), ChainEpoch(0));
}

/**
 * @given time just before epoch N start
 * @when epochAtTime
 * @then epoch N - 1
 */
TEST_F(ChainEpochClockTest, BeforeEpoch) {
  EXPECT_OUTCOME_EQ(epoch_clock.epochAtTime(kGenesisTime + kSecEpoch - kSec1),
                    ChainEpoch(kEpoch - 1));
}

/**
 * @given time at epoch N start
 * @when epochAtTime
 * @then epoch N
 */
TEST_F(ChainEpochClockTest, AtEpoch) {
  EXPECT_OUTCOME_EQ(epoch_clock.epochAtTime(kGenesisTime + kSecEpoch), kEpoch);
}

/**
 * @given time just after epoch N start
 * @when epochAtTime
 * @then epoch N
 */
TEST_F(ChainEpochClockTest, AfterEpoch) {
  EXPECT_OUTCOME_EQ(epoch_clock.epochAtTime(kGenesisTime + kSecEpoch + kSec1),
                    kEpoch);
}
