/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "power/power_table.hpp"

#include <gtest/gtest.h>

#include "power/impl/power_table_impl.hpp"
#include "power/power_table_error.hpp"
#include "testutil/outcome.hpp"

using fc::power::PowerTable;
using fc::power::PowerTableError;
using fc::power::PowerTableImpl;
using fc::primitives::address::Address;
using fc::primitives::address::Network;

class PowerTableTest : public ::testing::Test {
 public:
  PowerTableImpl power_table;
  Address addr{Address::makeFromId(3232104785)};
  int power{10};
};

/**
 * @given Empty power table
 * @when setting the negative power to miner
 * @then error NEGATIVE_POWER
 */
TEST_F(PowerTableTest, SetPower_NegativePower) {
  EXPECT_OUTCOME_ERROR(PowerTableError::NEGATIVE_POWER,
                       power_table.setMinerPower(addr, -1));
}

/**
 * @given Empty power table
 * @when setting the power to miner
 * @then power set successfully
 */
TEST_F(PowerTableTest, SetPower_Success) {
  EXPECT_OUTCOME_TRUE_1(power_table.setMinerPower(addr, power));
  EXPECT_OUTCOME_EQ(power_table.getMinerPower(addr), power);
}

/**
 * @given Empty power table
 * @when getting the power of the not existing miner
 * @then error NO_SUCH_MINER
 */
TEST_F(PowerTableTest, GetPower_NoMiner) {
  EXPECT_OUTCOME_ERROR(PowerTableError::NO_SUCH_MINER,
                       power_table.getMinerPower(addr));
}

/**
 * @given Empty power table
 * @when remove not existing miner
 * @then error NO_SUCH_MINER
 */
TEST_F(PowerTableTest, RemoveMiner_NoMiner) {
  EXPECT_OUTCOME_ERROR(PowerTableError::NO_SUCH_MINER,
                       power_table.removeMiner(addr));
}

/**
 * @given table with 1 miner
 * @when remove miner
 * @then miner successfully removed
 */
TEST_F(PowerTableTest, RemoveMiner_Success) {
  EXPECT_OUTCOME_TRUE_1(power_table.setMinerPower(addr, power));
  EXPECT_OUTCOME_EQ(power_table.getMinerPower(addr), power);
  EXPECT_OUTCOME_TRUE_1(power_table.removeMiner(addr));
  EXPECT_OUTCOME_ERROR(PowerTableError::NO_SUCH_MINER,
                       power_table.getMinerPower(addr));
}
