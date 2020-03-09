/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/primitives/ticket/ticket.hpp"

#include <gtest/gtest.h>
#include "testutil/outcome.hpp"
#include "testutil/primitives/ticket/printer.hpp"
#include "testutil/primitives/ticket/ticket_generator.hpp"

struct EPostTicketTest : public ::testing::Test {
  using Ticket = fc::primitives::ticket::EPostTicket;
  using Proof = fc::primitives::ticket::EPostProof;

  TicketGenerator generator;
};

/**
 * @given a ticket
 * @when compare ticket to itself using operator==
 * @then result of comparison is true
 */
TEST_F(EPostTicketTest, SameTicketEqualSuccess) {
  auto t = generator.makeEPostTicket();
  ASSERT_TRUE(t == t) << print(t);
}

/**
 * @given 2 different tickets
 * @when compare them using operator==
 * @then result of comparison is false
 * @and repeat loop_number times
 */
TEST_F(EPostTicketTest, DIfferentTicketEqualFailure) {
  auto &&t1 = generator.makeEPostTicket();
  auto &&t2 = generator.makeEPostTicket();
  ASSERT_FALSE(t1 == t2) << print(t1) << print(t2);
}

/**
 * @given a proof
 * @when compare proof to itself using operator==
 * @then result of comparison is true
 */
TEST_F(EPostTicketTest, SameProofEqualSuccess) {
  auto p = generator.makeEPostProof(12u, 3u);
  ASSERT_EQ(p, p) << print(p);
}

/**
 * @given 2 different proofs
 * @when compare them using operator==
 * @then result of comparison is false
 * @and repeat loop_number times
 */
TEST_F(EPostTicketTest, DIfferentProofsEqualFailure) {
  auto &&p1 = generator.makeEPostProof(12u, 2u);
  auto &&p2 = generator.makeEPostProof(12u, 3u);
  ASSERT_FALSE(p1 == p2) << print(p1) << print(p2);
}
