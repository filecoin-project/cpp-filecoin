/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/market/actor.hpp"

#include <gtest/gtest.h>
#include "testutil/cbor.hpp"

using fc::primitives::address::Address;
using fc::primitives::piece::PaddedPieceSize;
using fc::vm::actor::builtin::market::ClientDealProposal;
using fc::vm::actor::builtin::market::DealProposal;
using fc::vm::actor::builtin::market::DealState;
using fc::vm::actor::builtin::market::State;

/// State cbor encoding
TEST(MarketActorCborTest, State) {
  expectEncodeAndReencode(
      State{
          .proposals = decltype(State::proposals){"010001020001"_cid},
          .states = decltype(State::states){"010001020002"_cid},
          .escrow_table = decltype(State::escrow_table){"010001020003"_cid},
          .locked_table = decltype(State::locked_table){"010001020004"_cid},
          .next_deal = 1,
          .deals_by_party = decltype(State::deals_by_party){"010001020005"_cid},
      },
      "86d82a4700010001020001d82a4700010001020002d82a4700010001020003d82a470001000102000401d82a4700010001020005"_unhex);
}

/// DealState cbor encoding
TEST(MarketActorCborTest, DealState) {
  expectEncodeAndReencode(DealState{1, 2, 3}, "83010203"_unhex);
}

/// ClientDealProposal cbor encoding
TEST(MarketActorCborTest, ClientDealProposal) {
  expectEncodeAndReencode(
      ClientDealProposal{
          .proposal =
              DealProposal{
                  .piece_cid = "010001020001"_cid,
                  .piece_size = PaddedPieceSize{1},
                  .client = Address::makeFromId(1),
                  .provider = Address::makeFromId(2),
                  .start_epoch = 2,
                  .end_epoch = 3,
                  .storage_price_per_epoch = 4,
                  .provider_collateral = 5,
                  .client_collateral = 6,
              },
          .client_signature = "DEAD"_unhex,
      },
      "8289d82a47000100010200010142000142000202034200044200054200064301dead"_unhex);
}
