/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/ticket/epost_ticket_codec.hpp"

#include <gtest/gtest.h>
#include "testutil/outcome.hpp"
#include "testutil/primitives/ticket_utils.hpp"

using fc::codec::cbor::CborDecodeStream;
using fc::codec::cbor::CborEncodeStream;
using fc::common::Buffer;
using fc::crypto::vrf::VRFHash;
using fc::crypto::vrf::VRFProof;
using fc::primitives::ticket::EPostTicket;

struct EPostTicketCodecTest : public ::testing::Test {
  using EPostTicket = fc::primitives::ticket::EPostTicket;

  void SetUp() override {
    auto partial =
        fc::common::Blob<32>::fromHex(
            "0101010101010101010101010101010101010101010101010101010101010101")
            .value();
    ticket1 = EPostTicket{partial, 12u, 34u};

    cbor_value =
        "8358200101010101010101010101010101010101010101010101010101010101010101"
        "0c1822";
  }

  TicketGenerator generator{};
  EPostTicket ticket1;
  std::string cbor_value;
};

/**
 * @given generated random EPostTicket instance
 * @when encode ticket using CBorEncodeStream
 * @and then decode using CBorDecodeStream
 * @then decoded instance equal to original ticket
 */
TEST_F(EPostTicketCodecTest, EncodeDecodeSuccess) {
  auto &&t1 = generator.makeEPostTicket();
  CborEncodeStream es{};
  ASSERT_NO_THROW(es << t1);
  CborDecodeStream ds{es.data()};
  EPostTicket t2{};
  ASSERT_NO_THROW(ds >> t2);
  ASSERT_EQ(t1, t2);
}

/**
 * @given a EPostTicket ticket instance
 * @and its cbor string encoded using lotus implementation
 * @when decode ticket using CBorDecodeStream
 * @then decoded instance is equal to original ticket
 */
TEST_F(EPostTicketCodecTest, DecodeFromLotusSuccess) {
  EXPECT_OUTCOME_TRUE(cbor_data, Buffer::fromHex(cbor_value));
  CborDecodeStream ds{cbor_data};
  EPostTicket ticket2{};
  ASSERT_NO_THROW(ds >> ticket2);
  ASSERT_EQ(ticket1, ticket2);
}
