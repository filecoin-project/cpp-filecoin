/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/ticket/ticket.hpp"

#include <gtest/gtest.h>
#include "common/hexutil.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/primitives/ticket/printer.hpp"
#include "testutil/primitives/ticket/ticket_generator.hpp"

using fc::common::hex_lower;
using fc::primitives::ticket::drawRandomness;

struct TicketTest : public ::testing::Test {
  using Ticket = fc::primitives::ticket::Ticket;
  using Hash256 = fc::common::Hash256;

  TicketGenerator generator;

  Ticket ticket_small{
      "010101010101010101010101010101010101010101010101010101010101010101"
      "010101010101010101010101010101010101010101010101010101010101010101"
      "010101010101010101010101010101010101010101010101010101010101"_blob96};

  Ticket ticket_big{
      "010101010101010101010101010101010101010101010101010101010101010101"
      "010101010101010101010101010101010101010101010101010101010101010101"
      "010101010101010101010101010101010101010101010101010101010102"_blob96};

  uint64_t round_value = 2u;
  Hash256 randomness_value =
      "889fe0f04d131396b72d156e4d868cefc2c0aa50ffc7b94f6d873ed11a4ed8f1"_blob32;
};

/**
 * @given a ticket
 * @when compare ticket to itself using operator==
 * @then result of comparison is true
 */
TEST_F(TicketTest, SameTicketEqualSuccess) {
  auto t = generator.makeTicket();
  ASSERT_TRUE(t == t) << print(t);
}

/**
 * @given 2 different tickets
 * @when compare them using operator==
 * @then result of comparison is false
 */
TEST_F(TicketTest, DIfferentTicketEqualFailure) {
  auto &&t1 = generator.makeTicket();
  auto &&t2 = generator.makeTicket();
  ASSERT_FALSE(t1 == t2) << print(t1) << print(t2);
  ;
}

/**
 * @given two different tickets t1, t2
 * @when compare them using operator< (`less`)
 * @then one and only one of t1 < t2 and t2 < t1 is true
 */
TEST_F(TicketTest, EnsureLessIsAntisimmetricSuccess) {
  auto &&t1 = generator.makeTicket();
  auto &&t2 = generator.makeTicket();
  ASSERT_TRUE((t1 < t2) != (t2 < t1)) << print(t1) << print(t2);
}

/**
 * @given a random generated ticket
 * @when compare it to itself using operator< (`less`)
 * @then the result is false
 */
TEST_F(TicketTest, EnsureLessIsAntireflexiveSuccess) {
  auto &&t = generator.makeTicket();
  ASSERT_FALSE(t < t) << print(t);
}

/**
 * @given two tickets, for which it is known that one is less than other if
 * compared alphabetically
 * @when compare them using operator<(`less`)
 * @then the result of comparison is true
 */
TEST_F(TicketTest, TicketsCompareLessSuccess) {
  ASSERT_TRUE(ticket_small < ticket_big);
}

/**
 * @given a ticket and a round value
 * @when `drawRamdomness` method is applied
 * @then result is success
 */
TEST_F(TicketTest, DrawRandomnessSuccess) {
  auto &&t1 = generator.makeTicket();
  EXPECT_OUTCOME_TRUE_1(drawRandomness(t1, 1u));
}

/**
 * @given predefined ticket, round value and lotus-drawn randomness
 * @when call drawRandomness using given ticket and round value
 * @then resulting randomness value is equal to lotus-calculated value
 */
TEST_F(TicketTest, DrawRandomnessLotusSuccess) {
  EXPECT_OUTCOME_TRUE(randomness, drawRandomness(ticket_small, round_value));
  ASSERT_EQ(randomness, randomness_value);
}
