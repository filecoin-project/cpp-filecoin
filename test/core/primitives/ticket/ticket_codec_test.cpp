/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/ticket/ticket_codec.hpp"

#include <gtest/gtest.h>
#include "testutil/outcome.hpp"
#include "testutil/primitives/ticket/printer.hpp"
#include "testutil/primitives/ticket/ticket_generator.hpp"

using fc::codec::cbor::CborDecodeStream;
using fc::codec::cbor::CborEncodeStream;
using fc::common::Buffer;
using fc::crypto::vrf::VRFHash;
using fc::crypto::vrf::VRFProof;
using fc::primitives::ticket::Ticket;

/**
 * @struct TicketCodecLotusCrossTest test fixture for checking
 * if ticket cbor-marshal/unmarshal operations work exactly like lotus
 * implementation
 */
struct TicketCodecLotusCrossTest : public ::testing::Test {
  using Ticket = fc::primitives::ticket::Ticket;

  void SetUp() override {
    auto ticket_hex =
        fc::common::Blob<96>::fromHex(
            "010101010101010101010101010101010101010101010101010101010101010101"
            "010101010101010101010101010101010101010101010101010101010101010101"
            "010101010101010101010101010101010101010101010101010101010101")
            .value();
    ticket1 = Ticket{VRFProof(ticket_hex)};

    cbor_value =
        "8158600101010101010101010101010101010101010101010101010101010101010101"
        "0101010101010101010101010101010101010101010101010101010101010101010101"
        "0101010101010101010101010101010101010101010101010101010101";
  }

  Ticket ticket1;
  std::string cbor_value;
};

/**
 * @given a Ticket instance
 * @and its cbor string encoded using lotus implementation
 * @when decode ticket using CBorDecodeStream
 * @then decoded instance is equal to original ticket
 */
TEST_F(TicketCodecLotusCrossTest, DecodeFromLotusAndReencodeSuccess) {
  EXPECT_OUTCOME_TRUE(cbor_data, Buffer::fromHex(cbor_value));
  CborDecodeStream ds{cbor_data};
  Ticket ticket2{};
  ASSERT_NO_THROW(ds >> ticket2);
  ASSERT_EQ(ticket1, ticket2);
  CborEncodeStream es{};
  ASSERT_NO_THROW(es << ticket2);
  ASSERT_EQ(Buffer(es.data()), Buffer::fromHex(cbor_value).value());
}

/**
 * @struct TicketCodecRandomTest test ficture for working with generated tickets
 */
struct TicketCodecRandomTest : public ::testing::Test {
  using Ticket = fc::primitives::ticket::Ticket;
  const size_t loops_count = 10u;  //< number of times to run random test

  TicketGenerator generator{};
};

/**
 * @given generated random Ticket instance
 * @when encode ticket using CBorEncodeStream
 * @and then decode using CBorDecodeStream
 * @then decoded instance equal to original ticket
 * @and repeat tests_number times
 */
TEST_F(TicketCodecRandomTest, EncodeDecodeSuccess) {
  for (size_t i = 0; i < loops_count; ++i) {
    auto &&t1 = generator.makeTicket();
    CborEncodeStream es{};
    ASSERT_NO_THROW(es << t1) << print(t1);
    CborDecodeStream ds{es.data()};
    Ticket t2{};
    ASSERT_NO_THROW(ds >> t2) << print(t1) << print(t2);
    ASSERT_EQ(t1, t2) << print(t1) << print(t2);
  }
}
