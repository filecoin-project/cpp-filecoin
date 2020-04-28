/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "markets/storage/deal_protocol.hpp"
#include "testutil/cbor.hpp"
#include "testutil/literals.hpp"

namespace fc::markets::storage {

  using primitives::ChainEpoch;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::piece::PaddedPieceSize;

  /**
   * @given storage deal encoded in go-fil-markets
   * @when encode and decode
   * @then encoded bytes are equal to expected
   */
  TEST(StorageDealTest, EcodeAndDecode) {
    // DealProposal
    CID piece_cid = "010001020000"_cid;
    PaddedPieceSize piece_size{100};
    Address client = Address::makeFromId(1);
    Address provider = Address::makeFromId(2);
    ChainEpoch start_epoch{1};
    ChainEpoch end_epoch{2};
    TokenAmount storage_price_per_epoch{1};
    TokenAmount provider_collateral{2};
    TokenAmount client_collateral{3};

    // DealState parameters
    ChainEpoch sector_start_epoch{4};
    ChainEpoch last_updated_epoch{5};
    ChainEpoch slash_epoch{6};

    StorageDeal storage_deal{
        .proposal = {.piece_cid = piece_cid,
                          .piece_size = piece_size,
                          .client = client,
                          .provider = provider,
                          .start_epoch = start_epoch,
                          .end_epoch = end_epoch,
                          .storage_price_per_epoch = storage_price_per_epoch,
                          .provider_collateral = provider_collateral,
                          .client_collateral = client_collateral},
        .state = {.sector_start_epoch = sector_start_epoch,
                       .last_updated_epoch = last_updated_epoch,
                       .slash_epoch = slash_epoch}};

    std::vector<uint8_t> expected_bytes{
        "8289D82A47000100010200001864420001420002010242000142000242000383040506"_unhex};
    expectEncodeAndReencode(storage_deal, expected_bytes);
  }

}  // namespace fc::markets::storage
