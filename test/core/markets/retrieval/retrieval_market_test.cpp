/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core/markets/retrieval/fixture.hpp"
#include "proofs/proofs_error.hpp"
#include "testutil/mocks/std_function.hpp"
#include "testutil/outcome.hpp"

namespace fc::markets::retrieval::test {
  using fc::storage::ipld::kAllSelector;
  using primitives::piece::UnpaddedByteIndex;
  using primitives::sector::RegisteredSealProof;
  using primitives::sector::SectorId;
  using primitives::sector::SectorRef;
  using proofs::ProofsError;
  using testing::_;

  /**
   * @given Piece, which was stored to a Piece Storage
   * @when Sending QueryRequest to a provider
   * @then Provider must answer with QueryResponse with available item status
   */
  TEST_F(RetrievalMarketFixture, QuerySuccess) {
    EXPECT_CALL(*miner, getAddress())
        .WillOnce(testing::Return(Address::makeFromId(1000)));
    QueryRequest request{
        .payload_cid = payload_cid,
        .params = {.piece_cid = data::green_piece.info.piece_cid}};
    RetrievalPeer peer{.address = miner_worker_address,
                       .peer_id = host->getId(),
                       .piece = data::green_piece.info.piece_cid};
    MockStdFunction<client::QueryResponseHandler> cb;
    client->query(peer, request, cb.AsStdFunction());
    EXPECT_CALL(cb, Call(_)).WillOnce(testing::Invoke([&](auto _res) {
      EXPECT_OUTCOME_TRUE(res, _res);
      EXPECT_EQ(res.response_status,
                QueryResponseStatus::kQueryResponseAvailable);
      EXPECT_EQ(res.item_status, QueryItemStatus::kQueryItemAvailable);
      context->stop();
    }));
    context->run_for(std::chrono::minutes{1});
  }

  /**
   * @given retrieval provider has payload with cid
   * @when sending retrieval proposal
   * @then retrieval deal is committed and client receives requested data
   */
  TEST_F(RetrievalMarketFixture, RetrieveSuccess) {
    EXPECT_OUTCOME_EQ(client_ipfs->contains(payload_cid), false);

    auto sector_info = std::make_shared<mining::types::SectorInfo>();
    sector_info->sector_type = RegisteredSealProof::kStackedDrg2KiBV1;
    EXPECT_CALL(*miner, getSectorInfo(deal.sector_id))
        .WillOnce(testing::Return(outcome::success(sector_info)));

    const Address result_address = Address::makeFromId(1000);
    EXPECT_CALL(*miner, getAddress()).WillOnce(testing::Return(result_address));

    EXPECT_CALL(
        *sealer,
        readPiece(
            _,
            SectorRef{.id = SectorId{.miner = result_address.getId(),
                                     .sector = deal.sector_id},
                      .proof_type = RegisteredSealProof::kStackedDrg2KiBV1},
            UnpaddedByteIndex(deal.offset.unpadded()),
            deal.length.unpadded(),
            common::Hash256(),
            CID(),
            _,
            _))
        .WillOnce(testing::Invoke(
            [ipfs{provider_ipfs}, cid{payload_cid}](
                auto output, auto, auto, auto, auto, auto, auto cb, auto) {
              const auto output_fd = output.getFd();
              if (output_fd == -1) {
                cb(ProofsError::kCannotOpenFile);
              }
              EXPECT_OUTCOME_TRUE(car, fc::storage::car::makeCar(*ipfs, {cid}));
              auto bytes = write(output_fd, car.data(), car.size());
              if ((bytes < 0) || (static_cast<size_t>(bytes) != car.size())) {
                cb(ProofsError::kNotWriteEnough);
              }
              cb(true);
            }));

    DealProposalParams params{.selector = kAllSelector,
                              .piece = boost::none,
                              .price_per_byte = 2,
                              .payment_interval = 100,
                              .payment_interval_increase = 10};
    TokenAmount total_funds{100};
    std::promise<outcome::result<void>> retrieve_result;
    RetrievalPeer peer{.address = miner_worker_address,
                       .peer_id = host->getId(),
                       .piece = boost::none};
    MockStdFunction<client::RetrieveResponseHandler> cb;
    EXPECT_OUTCOME_TRUE_1(client->retrieve(payload_cid,
                                           params,
                                           total_funds,
                                           peer,
                                           client_wallet,
                                           miner_wallet,
                                           cb.AsStdFunction()));
    EXPECT_CALL(cb, Call(_)).WillOnce(testing::Invoke([&](auto _res) {
      EXPECT_OUTCOME_TRUE_1(_res);
      context->stop();
    }));
    context->run_for(std::chrono::minutes{1});
    EXPECT_OUTCOME_EQ(client_ipfs->contains(payload_cid), true);
  }
}  // namespace fc::markets::retrieval::test
