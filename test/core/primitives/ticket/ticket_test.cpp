/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/ticket/ticket.hpp"

#include <gtest/gtest.h>
#include "testutil/primitives/ticket_utils.hpp"
#include "testutil/outcome.hpp"

struct TicketTest : public ::testing::Test {
  using Ticket = fc::primitives::ticket::Ticket;

  TicketGenerator generator;
};

/**
 * @given a ticket
 * @when compare ticket to itself using operator==
 * @then result of comparison is true
 */
TEST_F(TicketTest, SameTicketEqualSuccess) {
  auto t = generator.makeTicket();
  ASSERT_TRUE(t == t);
}

/**
 * @given 2 different tickets
 * @when compare them using operator==
 * @then result of comparison is false
 */
TEST_F(TicketTest, DIfferentTicketEqualFailure) {
  auto &&t1 = generator.makeTicket();
  auto &&t2 = generator.makeTicket();
  ASSERT_FALSE(t1 == t2);
}

/**
 * @given two different tickets t1, t2
 * @when compare them using operator< (`less`)
 * @then one and only one of t1 < t2 and t2 < t1 is true
 */
TEST_F(TicketTest, TwoTicketsLessSuccess) {
  auto &&t1 = generator.makeTicket();
  auto &&t2 = generator.makeTicket();
  ASSERT_TRUE( (t1 < t2) != (t2 < t1));
}

/**
 * @given a ticket and a round value
 * @when drawRamdomness method is applied
 * @then result is success
 */
TEST_F(TicketTest, DrawRandomnessSuccess) {
  auto &&t1 = generator.makeTicket();
  EXPECT_OUTCOME_TRUE_1(fc::primitives::ticket::drawRandomness(t1, 1u));
}
