/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/vesting.hpp"

#include <gtest/gtest.h>

namespace fc::vm::actor::builtin::types::miner {

  struct VestingTest : testing::Test {
    void SetUp() override {
      vf.funds = {{.epoch = 100, .amount = 1000},
                  {.epoch = 101, .amount = 1100},
                  {.epoch = 102, .amount = 1200},
                  {.epoch = 103, .amount = 1300},
                  {.epoch = 104, .amount = 1400},
                  {.epoch = 105, .amount = 1500},
                  {.epoch = 106, .amount = 1600},
                  {.epoch = 107, .amount = 1700},
                  {.epoch = 108, .amount = 1800},
                  {.epoch = 109, .amount = 1900}};
    }

    VestingFunds vf;
    decltype(vf.funds) expected_funds;
  };

  TEST_F(VestingTest, UnlockVestedFundsEmptyFunds) {
    vf.funds.clear();

    const auto result = vf.unlockVestedFunds(100);
    EXPECT_EQ(result, 0);
    EXPECT_TRUE(vf.funds.empty());
  }

  TEST_F(VestingTest, UnlockVestedFundsNothingUnlocked) {
    expected_funds = {{.epoch = 100, .amount = 1000},
                      {.epoch = 101, .amount = 1100},
                      {.epoch = 102, .amount = 1200},
                      {.epoch = 103, .amount = 1300},
                      {.epoch = 104, .amount = 1400},
                      {.epoch = 105, .amount = 1500},
                      {.epoch = 106, .amount = 1600},
                      {.epoch = 107, .amount = 1700},
                      {.epoch = 108, .amount = 1800},
                      {.epoch = 109, .amount = 1900}};

    const auto result = vf.unlockVestedFunds(100);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(vf.funds, expected_funds);
  }

  TEST_F(VestingTest, UnlockVestedFunds) {
    expected_funds = {{.epoch = 105, .amount = 1500},
                      {.epoch = 106, .amount = 1600},
                      {.epoch = 107, .amount = 1700},
                      {.epoch = 108, .amount = 1800},
                      {.epoch = 109, .amount = 1900}};

    const auto result = vf.unlockVestedFunds(105);
    EXPECT_EQ(result, 1000 + 1100 + 1200 + 1300 + 1400);
    EXPECT_EQ(vf.funds, expected_funds);
  }

  TEST_F(VestingTest, UnlockVestedFundsUnlockAll) {
    expected_funds = {{.epoch = 100, .amount = 1000},
                      {.epoch = 101, .amount = 1100},
                      {.epoch = 102, .amount = 1200},
                      {.epoch = 103, .amount = 1300},
                      {.epoch = 104, .amount = 1400},
                      {.epoch = 105, .amount = 1500},
                      {.epoch = 106, .amount = 1600},
                      {.epoch = 107, .amount = 1700},
                      {.epoch = 108, .amount = 1800},
                      {.epoch = 109, .amount = 1900}};

    const auto result = vf.unlockVestedFunds(110);
    EXPECT_EQ(
        result,
        1000 + 1100 + 1200 + 1300 + 1400 + 1500 + 1600 + 1700 + 1800 + 1900);
    EXPECT_TRUE(vf.funds.empty());
  }

  TEST_F(VestingTest, AddLockedFundsEmptyFunds) {
    vf.funds.clear();

    expected_funds = {{.epoch = 101, .amount = 1000},
                      {.epoch = 102, .amount = 1000},
                      {.epoch = 103, .amount = 1000},
                      {.epoch = 104, .amount = 1000},
                      {.epoch = 105, .amount = 1000}};

    const VestSpec spec{.initial_delay = 0,
                        .vest_period = 5,
                        .step_duration = 1,
                        .quantization = 1};

    vf.addLockedFunds(100, 5000, 100, spec);

    EXPECT_EQ(vf.funds, expected_funds);
  }

  TEST_F(VestingTest, AddLockedFunds) {
    expected_funds = {{.epoch = 100, .amount = 1000},
                      {.epoch = 101, .amount = 1100},
                      {.epoch = 102, .amount = 3200},
                      {.epoch = 103, .amount = 1300},
                      {.epoch = 104, .amount = 3400},
                      {.epoch = 105, .amount = 1500},
                      {.epoch = 106, .amount = 3600},
                      {.epoch = 107, .amount = 1700},
                      {.epoch = 108, .amount = 3800},
                      {.epoch = 109, .amount = 1900},
                      {.epoch = 110, .amount = 2000}};

    const VestSpec spec{.initial_delay = 0,
                        .vest_period = 10,
                        .step_duration = 1,
                        .quantization = 2};

    vf.addLockedFunds(100, 10000, 100, spec);

    EXPECT_EQ(vf.funds, expected_funds);
  }

  TEST_F(VestingTest, UnlockUnvestedFundsEmptyFunds) {
    vf.funds.clear();

    const auto result = vf.unlockUnvestedFunds(100, 1000);
    EXPECT_EQ(result, 0);
    EXPECT_TRUE(vf.funds.empty());
  }

  TEST_F(VestingTest, UnlockUnvestedFundsUnlockBeginFunds) {
    expected_funds = {{.epoch = 101, .amount = 100},
                      {.epoch = 102, .amount = 1200},
                      {.epoch = 103, .amount = 1300},
                      {.epoch = 104, .amount = 1400},
                      {.epoch = 105, .amount = 1500},
                      {.epoch = 106, .amount = 1600},
                      {.epoch = 107, .amount = 1700},
                      {.epoch = 108, .amount = 1800},
                      {.epoch = 109, .amount = 1900}};

    const auto result = vf.unlockUnvestedFunds(100, 2000);
    EXPECT_EQ(result, 2000);
    EXPECT_EQ(vf.funds, expected_funds);
  }

  TEST_F(VestingTest, UnlockUnvestedFundsUnlockMiddleFunds) {
    expected_funds = {{.epoch = 100, .amount = 1000},
                      {.epoch = 101, .amount = 1100},
                      {.epoch = 102, .amount = 1200},
                      {.epoch = 103, .amount = 1300},
                      {.epoch = 106, .amount = 1500},
                      {.epoch = 107, .amount = 1700},
                      {.epoch = 108, .amount = 1800},
                      {.epoch = 109, .amount = 1900}};

    const auto result = vf.unlockUnvestedFunds(104, 3000);
    EXPECT_EQ(result, 3000);
    EXPECT_EQ(vf.funds, expected_funds);
  }

  TEST_F(VestingTest, UnlockUnvestedFundsUnlockEndFunds) {
    expected_funds = {{.epoch = 100, .amount = 1000},
                      {.epoch = 101, .amount = 1100},
                      {.epoch = 102, .amount = 1200},
                      {.epoch = 103, .amount = 1300},
                      {.epoch = 104, .amount = 1400},
                      {.epoch = 105, .amount = 1500},
                      {.epoch = 106, .amount = 1600}};

    const auto result = vf.unlockUnvestedFunds(107, 10000);
    EXPECT_EQ(result, 1700 + 1800 + 1900);
    EXPECT_EQ(vf.funds, expected_funds);
  }

  TEST_F(VestingTest, UnlockUnvestedFundsUnlockAllFunds) {
    const auto result = vf.unlockUnvestedFunds(100, 100000);
    EXPECT_EQ(
        result,
        1000 + 1100 + 1200 + 1300 + 1400 + 1500 + 1600 + 1700 + 1800 + 1900);
    EXPECT_TRUE(vf.funds.empty());
  }

  TEST_F(VestingTest, UnlockUnvestedFundsUnlockNothing) {
    expected_funds = {{.epoch = 100, .amount = 1000},
                      {.epoch = 101, .amount = 1100},
                      {.epoch = 102, .amount = 1200},
                      {.epoch = 103, .amount = 1300},
                      {.epoch = 104, .amount = 1400},
                      {.epoch = 105, .amount = 1500},
                      {.epoch = 106, .amount = 1600},
                      {.epoch = 107, .amount = 1700},
                      {.epoch = 108, .amount = 1800},
                      {.epoch = 109, .amount = 1900}};

    const auto result = vf.unlockUnvestedFunds(110, 2000);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(vf.funds, expected_funds);
  }

}  // namespace fc::vm::actor::builtin::types::miner
