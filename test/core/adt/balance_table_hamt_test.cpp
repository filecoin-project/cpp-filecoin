/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adt/balance_table_hamt.hpp"

#include <gtest/gtest.h>
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/outcome.hpp"

using fc::CID;
using fc::adt::BalanceTableHamt;
using fc::adt::TokenAmount;
using fc::primitives::address::Address;
using fc::storage::hamt::Hamt;
using fc::storage::hamt::HamtError;
using fc::storage::ipfs::InMemoryDatastore;
using fc::storage::ipfs::IpfsDatastore;

class BalanceTableHamtTest : public ::testing::Test {
 public:
  std::shared_ptr<IpfsDatastore> datastore =
      std::make_shared<InMemoryDatastore>();
  std::shared_ptr<BalanceTableHamt> table;
  Address address{Address::makeFromId(123)};

  void SetUp() override {
    Hamt empty_hamt(datastore);
    CID empty_cid = empty_hamt.flush().value();
    table = std::make_shared<BalanceTableHamt>(datastore, empty_cid);
  }
};

/**
 * @given a balance table is empty
 * @when try access to address
 * @then returns error address not found
 */
TEST_F(BalanceTableHamtTest, SubtractWithMinimumNotFound) {
  EXPECT_OUTCOME_ERROR(HamtError::NOT_FOUND,
                       table->subtractWithMinimum(address, 0, 0));
}

/**
 * @given a balance table with record and balance < floor
 * @when subtract is called
 * @then balance is not changed, 0 returned
 */
TEST_F(BalanceTableHamtTest, SubtractWithMinimumUnderFloor) {
  TokenAmount balance = 10;
  TokenAmount floor = 1000;
  TokenAmount to_subtract = 12;
  TokenAmount subtrahend = 0;
  TokenAmount difference = balance;
  EXPECT_OUTCOME_TRUE_1(table->set(address, balance));
  EXPECT_OUTCOME_EQ(table->subtractWithMinimum(address, to_subtract, floor),
                    subtrahend);
  EXPECT_OUTCOME_EQ(table->get(address), difference);
}

/**
 * @given a balance table with record and balance
 * @when subtract is called with subtrahend more than floor
 * @then balance changed, amount subtracted returned
 */
TEST_F(BalanceTableHamtTest, SubtractWithMinimumFloor) {
  TokenAmount balance = 100;
  TokenAmount floor = 50;
  TokenAmount to_subtract = 90;
  TokenAmount subtrahend = balance - floor;
  TokenAmount difference = floor;
  EXPECT_OUTCOME_TRUE_1(table->set(address, balance));
  EXPECT_OUTCOME_EQ(table->subtractWithMinimum(address, to_subtract, floor),
                    subtrahend);
  EXPECT_OUTCOME_EQ(table->get(address), difference);
}

/**
 * @given a balance table with record and balance
 * @when subtract is called with subtrahend more than floor
 * @then balance changed, amount subtracted returned
 */
TEST_F(BalanceTableHamtTest, SubtractWithMinimum) {
  TokenAmount balance = 100;
  TokenAmount floor = 50;
  TokenAmount to_subtract = 10;
  TokenAmount subtrahend = to_subtract;
  TokenAmount difference = balance - to_subtract;
  EXPECT_OUTCOME_TRUE_1(table->set(address, balance));
  EXPECT_OUTCOME_EQ(table->subtractWithMinimum(address, to_subtract, floor),
                    subtrahend);
  EXPECT_OUTCOME_EQ(table->get(address), difference);
}
