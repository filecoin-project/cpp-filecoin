/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "markets/retrieval/protocols/query_protocol.hpp"
#include "testutil/cbor.hpp"

namespace fc::markets::retrieval {
  using primitives::address::decodeFromString;

  /**
   * Tests retrieval market deal protocol V0.0.1.
   * Expected encoded bytes are from go-fil-markets implementation (commit:
   * b1a66cfd12686a8af6030fccace49916849b1954).
   */
  class QueryProtocolV0_0_1Test : public ::testing::Test {
   public:
    // address from go test constants
    Address address =
        decodeFromString("t2i4llai5x72clnz643iydyplvjmni74x4vyme7ny").value();

    // CID from go-generated string
    CID cid = CID::fromString("QmTTA2daxGqo5denp6SwLzzkLJm3fuisYEi9CoWsuHpzfb")
                  .value();
  };

  /**
   * Compatible with go lotus encoding with optional parameter is not set.
   */
  TEST_F(QueryProtocolV0_0_1Test, QueryWithoutParams) {
    QueryParams params{.piece_cid = boost::none};
    QueryRequestV0_0_1 request{{.payload_cid = cid, .params = params}};

    const auto go_encoded =
        "82d82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a81f6"_unhex;
    expectEncodeAndReencode(request, go_encoded);
  }

  /**
   * Compatible with go lotus encoding.
   */
  TEST_F(QueryProtocolV0_0_1Test, Query) {
    QueryParams params{.piece_cid = cid};
    QueryRequestV0_0_1 request{{.payload_cid = cid, .params = params}};

    const auto go_encoded =
        "82d82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a81d82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a"_unhex;
    expectEncodeAndReencode(request, go_encoded);
  }

  /**
   * Compatible with go lotus encoding.
   */
  TEST_F(QueryProtocolV0_0_1Test, QueryResponse) {
    QueryResponseV0_0_1 response{{
        .response_status = QueryResponseStatus::kQueryResponseAvailable,
        .item_status = QueryItemStatus::kQueryItemAvailable,
        .item_size = 64,
        .payment_address = address,
        .min_price_per_byte = 22,
        .payment_interval = 999,
        .interval_increase = 100,
        .message = "message",
        .unseal_price = 33,
    }};

    const auto go_encoded =
        "890000184055024716b023b7fe84b6e7dcda303c3d754b1a8ff2fc4200161903e71864676d657373616765420021"_unhex;
    expectEncodeAndReencode(response, go_encoded);
  }

}  // namespace fc::markets::retrieval
