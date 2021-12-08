/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <gtest/gtest.h>

#include "markets/retrieval/protocols/retrieval_protocol.hpp"
#include "testutil/cbor.hpp"
#include "vm/actor/builtin/types/payment_channel/voucher.hpp"

namespace fc::markets::retrieval {
  using primitives::address::decodeFromString;
  using storage::ipld::kAllSelector;
  using vm::actor::builtin::types::payment_channel::SignedVoucher;

  /**
   * Tests retrieval market deal protocol V0.0.1.
   * Expected encoded bytes are from go-fil-markets implementation (commit:
   * b1a66cfd12686a8af6030fccace49916849b1954).
   */
  class RetrievalProtocolV0_0_1Test : public ::testing::Test {
   public:
    // address from go test constants
    Address address =
        decodeFromString("t2i4llai5x72clnz643iydyplvjmni74x4vyme7ny").value();

    // CID from go-generated string
    CID cid = CID::fromString("QmTTA2daxGqo5denp6SwLzzkLJm3fuisYEi9CoWsuHpzfb")
                  .value();
  };

  /**
   * Compatible with go lotus encoding.
   */
  TEST_F(RetrievalProtocolV0_0_1Test, DealProposal) {
    DealProposalParams params{
        .selector = kAllSelector,
        .piece = cid,
        .price_per_byte = 12,
        .payment_interval = 222,
        .payment_interval_increase = 33,
        .unseal_price = 23,
    };
    DealProposalV0_0_1 proposal(cid, 12, params);

    EXPECT_EQ(proposal.type, "RetrievalDealProposal");

    const auto go_encoded =
        "83d82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a0c86a16152a2616ca1646e6f6e65a0623a3ea16161a1613ea16140a0d82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a42000c18de1821420017"_unhex;
    expectEncodeAndReencode(proposal, go_encoded);
  }

  /**
   * Compatible with go lotus encoding.
   */
  TEST_F(RetrievalProtocolV0_0_1Test, DealResponse) {
    DealResponseV0_0_1 response{{.status = DealStatus::kDealStatusAccepted,
                                 .deal_id = 12,
                                 .payment_owed = 222,
                                 .message = "message"}};
    EXPECT_EQ(response.type, "RetrievalDealResponse");

    const auto go_encoded = "84060c4200de676d657373616765"_unhex;
    expectEncodeAndReencode(response, go_encoded);
  }

  /**
   * Compatible with go lotus encoding.
   */
  TEST_F(RetrievalProtocolV0_0_1Test, DealPayment) {
    SignedVoucher voucher{
        .channel = address,
        .time_lock_min = 100,
        .time_lock_max = 200,
        .secret_preimage = "1234"_unhex,
        .extra = boost::none,
        .lane = 42,
        .nonce = 1,
        .amount = 22,
        .min_close_height = 333,
        .merges = {},
        .signature_bytes = boost::none,
    };

    DealPaymentV0_0_1 payment{{
        .deal_id = 12,
        .payment_channel = address,
        .payment_voucher = voucher,
    }};

    EXPECT_EQ(payment.type, "RetrievalDealPayment");

    const auto go_encoded =
        "830c55024716b023b7fe84b6e7dcda303c3d754b1a8ff2fc8b55024716b023b7fe84b6e7dcda303c3d754b1a8ff2fc186418c8421234f6182a0142001619014d80f6"_unhex;
    expectEncodeAndReencode(payment, go_encoded);
  }
}  // namespace fc::markets::retrieval
