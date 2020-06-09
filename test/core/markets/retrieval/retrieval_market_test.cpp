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
    OUTCOME_EXCEPT(response, client->query(host->getPeerInfo(), request))
    ASSERT_EQ(response->response_status,
              QueryResponseStatus::QueryResponseAvailable);
    ASSERT_EQ(response->item_status, QueryItemStatus::QueryItemAvailable);
  }
}  // namespace fc::markets::retrieval::test
