/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clock/impl/chain_epoch_clock_impl.hpp"

#include <gtest/gtest.h>
#include "testutil/outcome.hpp"

using fc::clock::Time;
using fc::clock::UnixTimeNano;
using fc::clock::ChainEpoch;
using fc::clock::ChainEpochClock;
using fc::clock::kEpochDuration;
using fc::clock::ChainEpochClockImpl;

static Time kGenesisTime(UnixTimeNano(1));
static std::chrono::nanoseconds kNano1(1);
static ChainEpoch kEpoch(3);
static std::chrono::nanoseconds kNanoEpoch(kEpoch.toUInt64() * kEpochDuration);

class ChainEpochClockTest : public ::testing::Test {
 public:
  void SetUp() override {
    chain_epoch_clock_ = std::make_unique<ChainEpochClockImpl>(kGenesisTime);
  }

  fc::outcome::result<ChainEpoch> epochAtTime(const std::chrono::nanoseconds &nano) const {
    return chain_epoch_clock_->epochAtTime(Time(nano));
  }

  std::unique_ptr<ChainEpochClock> chain_epoch_clock_;
};

/**
 * @given genesis time
 * @when construct ChainEpochClock and get genesisTime
 * @then equals to original
 */
TEST_F(ChainEpochClockTest, GenesisTime) {
  EXPECT_EQ(chain_epoch_clock_->genesisTime(), kGenesisTime);
}

/**
 * @given time just before genesis
 * @when epochAtTime
 * @then error
 */
TEST_F(ChainEpochClockTest, BeforeGenesis) {
  auto epoch_at_time = epochAtTime(kGenesisTime.unixTimeNano() - kNano1);

  EXPECT_OUTCOME_FALSE_1(epoch_at_time);
}

/**
 * @given time at genesis
 * @when epochAtTime
 * @then epoch 0
 */
TEST_F(ChainEpochClockTest, AtGenesis) {
  auto epoch = chain_epoch_clock_->epochAtTime(kGenesisTime).value();

  EXPECT_EQ(epoch, ChainEpoch(0));
}

/**
 * @given time just before epoch N start
 * @when epochAtTime
 * @then epoch N - 1
 */
TEST_F(ChainEpochClockTest, BeforeEpoch) {
  auto epoch = epochAtTime(kGenesisTime.unixTimeNano() + kNanoEpoch - kNano1).value();

  EXPECT_EQ(epoch, ChainEpoch(kEpoch.toUInt64() - 1));
}

/**
 * @given time at epoch N start
 * @when epochAtTime
 * @then epoch N
 */
TEST_F(ChainEpochClockTest, AtEpoch) {
  auto epoch = epochAtTime(kGenesisTime.unixTimeNano() + kNanoEpoch).value();

  EXPECT_EQ(epoch, kEpoch);
}

/**
 * @given time just after epoch N start
 * @when epochAtTime
 * @then epoch N
 */
TEST_F(ChainEpochClockTest, AfterEpoch) {
  auto epoch = epochAtTime(kGenesisTime.unixTimeNano() + kNanoEpoch + kNano1).value();

  EXPECT_EQ(epoch, kEpoch);
}
