/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "power/impl/power_table_hamt.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <power/power_table_error.hpp>
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/outcome.hpp"

using fc::power::PowerTableError;
using fc::power::PowerTableHamt;
using fc::primitives::address::Address;
using fc::primitives::address::Network;
using fc::storage::hamt::Hamt;
using fc::storage::ipfs::InMemoryDatastore;
using fc::storage::ipfs::IpfsDatastore;

class PowerTableHamtTest : public ::testing::Test {
 public:
  std::shared_ptr<IpfsDatastore> datastore =
      std::make_shared<InMemoryDatastore>();
  Hamt hamt{datastore};
  PowerTableHamt power_table{hamt};
  Address addr{Address::makeFromId(3232104785)};
  Address addr1{Address::makeFromId(111)};
  Address addr2{Address::makeFromId(2222)};
  int power{10};
};

/**
 * @given Empty power table
 * @when setting the negative power to miner
 * @then error NEGATIVE_POWER
 */
TEST_F(PowerTableHamtTest, SetPower_NegativePower) {
  EXPECT_OUTCOME_ERROR(PowerTableError::NEGATIVE_POWER,
                       power_table.setMinerPower(addr, -1));
}

/**
 * @given Empty power table
 * @when setting the power to miner
 * @then power set successfully
 */
TEST_F(PowerTableHamtTest, SetPower_Success) {
  EXPECT_OUTCOME_TRUE_1(power_table.setMinerPower(addr, power));
  EXPECT_OUTCOME_EQ(power_table.getMinerPower(addr), power);
}

/**
 * @given Empty power table
 * @when getting the power of the not existing miner
 * @then error NO_SUCH_MINER
 */
TEST_F(PowerTableHamtTest, GetPower_NoMiner) {
  EXPECT_OUTCOME_ERROR(PowerTableError::NO_SUCH_MINER,
                       power_table.getMinerPower(addr));
}

/**
 * @given Empty power table
 * @when remove not existing miner
 * @then error NO_SUCH_MINER
 */
TEST_F(PowerTableHamtTest, RemoveMiner_NoMiner) {
  EXPECT_OUTCOME_ERROR(PowerTableError::NO_SUCH_MINER,
                       power_table.removeMiner(addr));
}

/**
 * @given table with 1 miner
 * @when remove miner
 * @then miner successfully removed
 */
TEST_F(PowerTableHamtTest, RemoveMiner_Success) {
  EXPECT_OUTCOME_TRUE_1(power_table.setMinerPower(addr, power));
  EXPECT_OUTCOME_EQ(power_table.getMinerPower(addr), power);
  EXPECT_OUTCOME_TRUE_1(power_table.removeMiner(addr));
  EXPECT_OUTCOME_ERROR(PowerTableError::NO_SUCH_MINER,
                       power_table.getMinerPower(addr));
}

/**
 * @given empty table
 * @when size called
 * @then correct number of elements in map returned
 */
TEST_F(PowerTableHamtTest, GetSize) {
  EXPECT_OUTCOME_EQ(power_table.getSize(), 0);
  EXPECT_OUTCOME_TRUE_1(power_table.setMinerPower(addr, power));
  EXPECT_OUTCOME_EQ(power_table.getSize(), 1);
  EXPECT_OUTCOME_TRUE_1(power_table.setMinerPower(addr1, power));
  EXPECT_OUTCOME_EQ(power_table.getSize(), 2);
  EXPECT_OUTCOME_TRUE_1(power_table.setMinerPower(addr2, power));
  EXPECT_OUTCOME_EQ(power_table.getSize(), 3);
  EXPECT_OUTCOME_TRUE_1(power_table.removeMiner(addr));
  EXPECT_OUTCOME_EQ(power_table.getSize(), 2);
  EXPECT_OUTCOME_TRUE_1(power_table.removeMiner(addr1));
  EXPECT_OUTCOME_EQ(power_table.getSize(), 1);
  EXPECT_OUTCOME_TRUE_1(power_table.removeMiner(addr2));
  EXPECT_OUTCOME_EQ(power_table.getSize(), 0);
}

/**
 * @given empty table
 * @when get max power
 * @then 0 returned
 */
TEST_F(PowerTableHamtTest, GetMaxPowerEmpty) {
  EXPECT_OUTCOME_EQ(power_table.getMaxPower(), 0);
}

/**
 * @given empty table
 * @when get max power
 * @then 0 returned
 */
TEST_F(PowerTableHamtTest, GetMaxPowerSuccess) {
  int smaller = 1;
  int small = 20;
  int max = 300;
  EXPECT_OUTCOME_TRUE_1(power_table.setMinerPower(addr, small));
  EXPECT_OUTCOME_TRUE_1(power_table.setMinerPower(addr1, smaller));
  EXPECT_OUTCOME_TRUE_1(power_table.setMinerPower(addr2, max));
  EXPECT_OUTCOME_EQ(power_table.getMaxPower(), max);
}

/**
 * @given empty table
 * @when get miners
 * @then empty set returned
 */
TEST_F(PowerTableHamtTest, ) {
  std::vector<Address> empty;
  EXPECT_OUTCOME_EQ(power_table.getMiners(), empty);
}

/**
 * @given populated table
 * @when get miners
 * @then all miners returned
 */
TEST_F(PowerTableHamtTest, GetMinersSuccess) {
  EXPECT_OUTCOME_TRUE_1(power_table.setMinerPower(addr, power));
  EXPECT_OUTCOME_TRUE_1(power_table.setMinerPower(addr1, power));
  EXPECT_OUTCOME_TRUE_1(power_table.setMinerPower(addr2, power));

  EXPECT_OUTCOME_TRUE(res, power_table.getMiners());
  ASSERT_THAT(res, ::testing::UnorderedElementsAre(addr, addr1, addr2));
}
