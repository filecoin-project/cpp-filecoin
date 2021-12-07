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
  class RetrievalProtocolV1_0_0Test : public ::testing::Test {
   public:
    // CID from go-generated string
    CID cid = CID::fromString("QmTTA2daxGqo5denp6SwLzzkLJm3fuisYEi9CoWsuHpzfb")
                  .value();
  };

  /**
   * Compatible with go lotus encoding.
   */
  TEST_F(RetrievalProtocolV1_0_0Test, DealProposal) {
    DealProposalParams params{
        .selector = kAllSelector,
        .piece = cid,
        .price_per_byte = 12,
        .payment_interval = 222,
        .payment_interval_increase = 33,
        .unseal_price = 23,
    };
    DealProposalV1_0_0 proposal(cid, 12, params);

    EXPECT_EQ(proposal.type, "RetrievalDealProposal/1");

    const auto go_encoded =
        "a36a5061796c6f6164434944d82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a6249440c66506172616d73a66853656c6563746f72a16152a2616ca1646e6f6e65a0623a3ea16161a1613ea16140a0685069656365434944d82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a6c50726963655065724279746542000c6f5061796d656e74496e74657276616c18de775061796d656e74496e74657276616c496e63726561736518216b556e7365616c5072696365420017"_unhex;
    expectEncodeAndReencode(proposal, go_encoded);
  }

  TEST_F(RetrievalProtocolV1_0_0Test, DealResponse) {
    DealResponseV1_0_0 response{{.status = DealStatus::kDealStatusAccepted,
                                 .deal_id = 12,
                                 .payment_owed = 222,
                                 .message = "message"}};
    EXPECT_EQ(response.type, "RetrievalDealResponse/1");

    const auto go_encoded = "a466537461747573066249440c6b5061796d656e744f7765644200de674d657373616765676d657373616765"_unhex;
    expectEncodeAndReencode(response, go_encoded);
  }
}  // namespace fc::markets::retrieval