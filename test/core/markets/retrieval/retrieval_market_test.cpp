/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core/markets/retrieval/fixture.hpp"
#include "testutil/outcome.hpp"

namespace fc::markets::retrieval::test {
  using fc::storage::ipld::kAllSelector;

  /**
   * @given Piece, which was stored to a Piece Storage
   * @when Sending QueryRequest to a provider
   * @then Provider must answer with QueryResponse with available item status
   */
  TEST_F(RetrievalMarketFixture, QuerySuccess) {
    QueryRequest request{
        .payload_cid = payload_cid,
        .params = {.piece_cid = data::green_piece.info.piece_cid}};
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
              QueryResponseStatus::kQueryResponseAvailable);
    EXPECT_EQ(response_res.value().item_status,
              QueryItemStatus::kQueryItemAvailable);
  }

  /**
   * @given retrieval provider has payload with cid
   * @when sending retrieval proposal
   * @then retrieval deal is committed and client receives requested data
   */
  TEST_F(RetrievalMarketFixture, RetrieveSuccess) {
    EXPECT_OUTCOME_EQ(client_ipfs->contains(payload_cid), false);

    DealProposalParams params{.selector = kAllSelector,
                              .piece = boost::none,
                              .price_per_byte = 2,
                              .payment_interval = 100,
                              .payment_interval_increase = 10};
    TokenAmount total_funds{100};
    std::promise<outcome::result<void>> retrieve_result;
    client->retrieve(
        payload_cid,
        params,
        host->getPeerInfo(),
        client_wallet,
        miner_wallet,
        total_funds,
        [&](outcome::result<void> res) { retrieve_result.set_value(res); });
    auto future = retrieve_result.get_future();
    ASSERT_EQ(future.wait_for(std::chrono::seconds(5)),
              std::future_status::ready);
    EXPECT_OUTCOME_TRUE_1(future.get());

    EXPECT_OUTCOME_EQ(client_ipfs->contains(payload_cid), true);
  }
}  // namespace fc::markets::retrieval::test
