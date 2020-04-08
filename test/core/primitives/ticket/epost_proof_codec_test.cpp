/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/ticket/epost_ticket_codec.hpp"

#include <gtest/gtest.h>

#include "testutil/cbor.hpp"
#include "testutil/primitives/ticket/printer.hpp"
#include "testutil/primitives/ticket/ticket_generator.hpp"

using fc::codec::cbor::CborDecodeStream;
using fc::codec::cbor::CborEncodeStream;
using fc::common::Buffer;
using fc::crypto::vrf::VRFHash;
using fc::crypto::vrf::VRFProof;
using fc::crypto::vrf::VRFResult;
using fc::primitives::sector::PoStProof;
using fc::primitives::sector::RegisteredProof;
using fc::primitives::ticket::EPostProof;
using fc::primitives::ticket::EPostTicket;
using fc::primitives::ticket::PostRandomness;

TEST(EPostProofCborTest, EPostProof) {
  auto partial =
      "0101010101010101010101010101010101010101010101010101010101010101"_blob32;
  expectEncodeAndReencode(
      EPostProof{
          {PoStProof{RegisteredProof::StackedDRG2KiBSeal,
                     "01020304050607080900"_unhex}},
          "010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101"_blob96,
          {
              EPostTicket{partial, 12u, 34u},
              EPostTicket{partial, 21u, 43u},
          },
      },
      "838182034a0102030405060708090058600101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101018283582001010101010101010101010101010101010101010101010101010101010101010c1822835820010101010101010101010101010101010101010101010101010101010101010115182b"_unhex);
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
