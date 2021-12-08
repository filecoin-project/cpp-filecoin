/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "markets/retrieval/protocols/query_protocol.hpp"
#include "testutil/cbor.hpp"

namespace fc::markets::retrieval {
  using primitives::address::decodeFromString;

  class QueryProtocolV1_0_0Test : public ::testing::Test {
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
  TEST_F(QueryProtocolV1_0_0Test, QueryWithoutParams) {
    QueryParams params{.piece_cid = boost::none};
    QueryRequestV1_0_0 request{{.payload_cid = cid, .params = params}};

    const auto go_encoded =
        "a26a5061796c6f6164434944d82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a6b5175657279506172616d73a1685069656365434944f6"_unhex;
    expectEncodeAndReencode(request, go_encoded);
  }

  /**
   * Compatible with go lotus encoding.
   */
  TEST_F(QueryProtocolV1_0_0Test, Query) {
    QueryParams params{.piece_cid = cid};
    QueryRequestV1_0_0 request{{.payload_cid = cid, .params = params}};

    const auto go_encoded =
        "a26a5061796c6f6164434944d82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a6b5175657279506172616d73a1685069656365434944d82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a"_unhex;
    expectEncodeAndReencode(request, go_encoded);
  }

  /**
   * Compatible with go lotus encoding.
   */
  TEST_F(QueryProtocolV1_0_0Test, QueryResponse) {
    QueryResponseV1_0_0 response{{
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
        "a966537461747573006d5069656365434944466f756e64006453697a6518406e5061796d656e744164647265737355024716b023b7fe84b6e7dcda303c3d754b1a8ff2fc6f4d696e507269636550657242797465420016724d61785061796d656e74496e74657276616c1903e7781a4d61785061796d656e74496e74657276616c496e6372656173651864674d657373616765676d6573736167656b556e7365616c5072696365420021"_unhex;
    expectEncodeAndReencode(response, go_encoded);
  }

}  // namespace fc::markets::retrieval
