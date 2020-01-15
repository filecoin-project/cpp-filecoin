/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/ticket/epost_ticket_codec.hpp"

#include <gtest/gtest.h>
#include "testutil/outcome.hpp"
#include "testutil/primitives/ticket/printer.hpp"
#include "testutil/primitives/ticket/ticket_generator.hpp"

using fc::codec::cbor::CborDecodeStream;
using fc::codec::cbor::CborEncodeStream;
using fc::common::Buffer;
using fc::crypto::vrf::VRFHash;
using fc::crypto::vrf::VRFProof;
using fc::crypto::vrf::VRFResult;
using fc::primitives::ticket::EPostTicket;
using fc::primitives::ticket::PostRandomness;

/**
 * @struct EPostProofCodecLotusCrossTest test fixture for checking
 * if proof cbor-marshal/unmarshal operations work exactly like lotus
 * implementation
 */

struct EPostProofCodecLotusCrossTest : public ::testing::Test {
  using EPostProof = fc::primitives::ticket::EPostProof;

  void SetUp() override {
    auto proof = Buffer::fromHex("01020304050607080900").value();

    auto post_rand_hex =
        fc::common::Blob<96>::fromHex(
            "010101010101010101010101010101010101010101010101010101010101010101"
            "010101010101010101010101010101010101010101010101010101010101010101"
            "010101010101010101010101010101010101010101010101010101010101")
            .value();

    auto post_rand = PostRandomness{VRFResult{post_rand_hex}};
    auto partial =
        fc::common::Blob<32>::fromHex(
            "0101010101010101010101010101010101010101010101010101010101010101")
            .value();
    auto t1 = EPostTicket{partial, 12u, 34u};
    auto t2 = EPostTicket{partial, 21u, 43u};

    epp1 = EPostProof{proof, post_rand, {t1, t2}};

    cbor_value =
        "834a010203040506070809005860010101010101010101010101010101010101010101"
        "0101010101010101010101010101010101010101010101010101010101010101010101"
        "0101010101010101010101010101010101010101010101010101010101010101010101"
        "0101010101828358200101010101010101010101010101010101010101010101010101"
        "0101010101010c18228358200101010101010101010101010101010101010101010101"
        "01010101010101010115182b";
  }

  EPostProof epp1;
  std::string cbor_value;
};

/**
 * @given a EPostProof proof instance
 * @and its cbor string encoded using lotus implementation
 * @when decode proof using CBorDecodeStream
 * @then decoded instance is equal to original proof
 */
TEST_F(EPostProofCodecLotusCrossTest, DecodeFromLotusSuccess) {
  EXPECT_OUTCOME_TRUE(cbor_data, Buffer::fromHex(cbor_value));
  CborDecodeStream ds{cbor_data};

  EPostProof epp2{};
  ASSERT_NO_THROW(ds >> epp2);
  ASSERT_EQ(epp1, epp2);
  CborEncodeStream es{};
  ASSERT_NO_THROW(es << epp2);
  ASSERT_EQ(Buffer(es.data()), Buffer::fromHex(cbor_value).value());
}

/**
 * @struct EPostTicketCodecRandomTest test ficture for working with generated
 * proofs
 */
struct EPostProofCodecRandomTest : public ::testing::Test {
  using EPostProof = fc::primitives::ticket::EPostProof;

  size_t loops_count = 10u;  //< number of times to run random test

  TicketGenerator generator;
};

/**
 * @given generated random EPostProof instance
 * @when encode proof using CBorEncodeStream
 * @and then decode using CBorDecodeStream
 * @then decoded instance equal to original proof
 */
TEST_F(EPostProofCodecRandomTest, EncodeDecodeSuccess) {
  for (size_t i = 0; i < loops_count; ++i) {
    auto &&p1 = generator.makeEPostProof(32u, 2u);
    CborEncodeStream es{};
    ASSERT_NO_THROW(es << p1) << print(p1);
    CborDecodeStream ds{es.data()};
    EPostProof p2{};
    ASSERT_NO_THROW(ds >> p2) << print(p1) << print(p2);
    ASSERT_EQ(p1, p2) << print(p1) << print(p2);
  }
}
