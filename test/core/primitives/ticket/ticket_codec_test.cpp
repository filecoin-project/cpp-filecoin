/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/ticket/ticket_codec.hpp"

#include <gtest/gtest.h>
#include "testutil/outcome.hpp"
#include "testutil/primitives/ticket_utils.hpp"

using fc::codec::cbor::CborDecodeStream;
using fc::codec::cbor::CborEncodeStream;
using fc::common::Buffer;
using fc::crypto::vrf::VRFHash;
using fc::crypto::vrf::VRFProof;
using fc::primitives::ticket::Ticket;

struct TicketCodecTest : public ::testing::Test {
  using Ticket = fc::primitives::ticket::Ticket;

  TicketGenerator generator{};
};

TEST_F(TicketCodecTest, EncodeDecodeSuccess) {
  auto &&t1 = generator.makeTicket();
  CborEncodeStream es{};
  ASSERT_NO_THROW(es << t1);
  CborDecodeStream ds{es.data()};
  Ticket t2{};
  ASSERT_NO_THROW(ds >> t2);
  ASSERT_EQ(t1, t2);
}

TEST_F(TicketCodecTest, DecodeFromLotusSuccess) {
  auto hex_ticket =
      fc::common::Blob<96>::fromHex(
          "01010101010101010101010101010101010101010101010101010101010101010101"
          "01010101010101010101010101010101010101010101010101010101010101010101"
          "01010101010101010101010101010101010101010101010101010101")
          .value();
  auto ticket = Ticket{VRFProof(hex_ticket)};

  std::string cbor_value =
      "815860010101010101010101010101010101010101010101010101010101010101010101"
      "010101010101010101010101010101010101010101010101010101010101010101010101"
      "010101010101010101010101010101010101010101010101010101";

  EXPECT_OUTCOME_TRUE(cbor_data, Buffer::fromHex(cbor_value));

  CborDecodeStream ds{cbor_data};
  Ticket ticket1{};
  ASSERT_NO_THROW(ds >> ticket1);
  ASSERT_EQ(ticket1, ticket);
}
