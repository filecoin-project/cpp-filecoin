/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "power/power_table.hpp"

#include <gtest/gtest.h>

#include "power/impl/power_table_impl.hpp"
#include "power/power_table_error.hpp"
#include "testutil/outcome.hpp"

using fc::primitives::address::Address;
using fc::primitives::address::Network;
using fc::power::PowerTable;
using fc::power::PowerTableError;
using fc::power::PowerTableImpl;

/**
 * @given Empty power table
 * @when setting the negative power to miner
 * @then error NEGATIVE_POWER
 */
TEST(PowerTable, SetPower_NegativePower) {
  std::shared_ptr<PowerTable> power_table = std::make_shared<PowerTableImpl>();
  Address addrID_0{Network::MAINNET, 3232104785};
  EXPECT_OUTCOME_FALSE(err, power_table->setMinerPower(addrID_0, -1));
  ASSERT_EQ(err, PowerTableError::NEGATIVE_POWER);
}

/**
 * @given Empty power table
 * @when setting the power to miner
 * @then power set successfully
 */
TEST(PowerTable, SetPower_Success) {
  std::shared_ptr<PowerTable> power_table = std::make_shared<PowerTableImpl>();
  Address addrID_0{Network::MAINNET, 3232104785};
  int power = 10;
  EXPECT_OUTCOME_TRUE_1(power_table->setMinerPower(addrID_0, power));
  EXPECT_OUTCOME_TRUE(val, power_table->getMinerPower(addrID_0));
  ASSERT_EQ(val, power);
}

/**
 * @given Empty power table
 * @when getting the power of the not existing miner
 * @then error NO_SUCH_MINER
 */
TEST(PowerTable, GetPower_NoMiner) {
  std::shared_ptr<PowerTable> power_table = std::make_shared<PowerTableImpl>();
  Address addrID_0{Network::MAINNET, 3232104785};
  EXPECT_OUTCOME_FALSE(err, power_table->getMinerPower(addrID_0));
  ASSERT_EQ(err, PowerTableError::NO_SUCH_MINER);
}

/**
 * @given table with 1 miner
 * @when getting the power of the miner
 * @then power successfully received
 */
TEST(PowerTable, GetPower_Success) {
  std::shared_ptr<PowerTable> power_table = std::make_shared<PowerTableImpl>();
  Address addrID_0{Network::MAINNET, 3232104785};
  int power = 10;
  EXPECT_OUTCOME_TRUE_1(power_table->setMinerPower(addrID_0, power));
  EXPECT_OUTCOME_TRUE(res, power_table->getMinerPower(addrID_0));
  ASSERT_EQ(res, power);
}

/**
 * @given Empty power table
 * @when remove not existing miner
 * @then error NO_SUCH_MINER
 */
TEST(PowerTable, RemoveMiner_NoMiner) {
  std::shared_ptr<PowerTable> power_table = std::make_shared<PowerTableImpl>();
  Address addrID_0{Network::MAINNET, 3232104785};
  EXPECT_OUTCOME_FALSE(err, power_table->removeMiner(addrID_0));
  ASSERT_EQ(err, PowerTableError::NO_SUCH_MINER);
}

/**
 * @given table with 1 miner
 * @when remove miner
 * @then miner successfully removed
 */
TEST(PowerTable, RemoveMiner_Success) {
  std::shared_ptr<PowerTable> power_table = std::make_shared<PowerTableImpl>();
  Address addrID_0{Network::MAINNET, 3232104785};
  int power = 10;
  EXPECT_OUTCOME_TRUE_1(power_table->setMinerPower(addrID_0, power));
  EXPECT_OUTCOME_TRUE(res, power_table->getMinerPower(addrID_0));
  ASSERT_EQ(res, power);
  EXPECT_OUTCOME_TRUE_1(power_table->removeMiner(addrID_0));
  EXPECT_OUTCOME_FALSE(err, power_table->getMinerPower(addrID_0));
  ASSERT_EQ(err, PowerTableError::NO_SUCH_MINER);
}
