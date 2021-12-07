/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <gtest/gtest.h>

#include "markets/retrieval/protocols/retrieval_protocol.hpp"
#include "testutil/cbor.hpp"

namespace fc::markets::retrieval {
  using storage::ipld::kAllSelector;

  /**
   * Tests retrieval market deal protocol V0.0.1.
   * Expected encoded bytes are from go-fil-markets implementation (commit:
   * b1a66cfd12686a8af6030fccace49916849b1954).
   */
  class RetrievalProtocolV0_0_1Test : public ::testing::Test {
   public:
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

  TEST_F(RetrievalProtocolV0_0_1Test, DealResponse) {
    DealResponseV0_0_1 response{{.status = DealStatus::kDealStatusAccepted,
                                 .deal_id = 12,
                                 .payment_owed = 222,
                                 .message = "message"}};
    EXPECT_EQ(response.type, "RetrievalDealResponse");

    const auto go_encoded = "84060c4200de676d657373616765"_unhex;
    expectEncodeAndReencode(response, go_encoded);
  }
}  // namespace fc::markets::retrieval