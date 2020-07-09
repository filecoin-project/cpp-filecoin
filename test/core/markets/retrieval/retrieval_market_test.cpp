/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core/markets/retrieval/fixture.hpp"

namespace fc::markets::retrieval::test {
  /**
   * @given Piece, which was stored to a Piece Storage
   * @when Sending QueryRequest to a provider
   * @then Provider must answer with QueryResponse with available item status
   */
  TEST_F(RetrievalMarketFixture, QuerySuccess) {
    QueryRequest request{.payload_cid = data::green_piece.payloads[0].cid,
                         .params = {.piece_cid = data::green_piece.cid}};
    std::promise<outcome::result<QueryResponse>> query_result;
    client->query(host->getPeerInfo(), request, [&](auto response) {
      query_result.set_value(response);
    });
    auto future = query_result.get_future();
    ASSERT_EQ(future.wait_for(std::chrono::seconds(3)),
              std::future_status::ready);
    auto response_res = future.get();
    EXPECT_TRUE(response_res.has_value());
    EXPECT_EQ(response_res.value().response_status,
              QueryResponseStatus::kQueryResponseUnavailable);
    EXPECT_EQ(response_res.value().item_status,
              QueryItemStatus::kQueryItemUnavailable);
  }
}  // namespace fc::markets::retrieval::test
