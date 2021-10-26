/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/rle_bitset/rle_bitset.hpp"
#include "vm/actor/builtin/v3/market/market_actor.hpp"

// TODO(m.tagirov): implement market actor v6
namespace fc::vm::actor::builtin::v6::market {
  using primitives::DealId;
  using primitives::RleBitset;
  using types::market::ClientDealProposal;

  struct PublishStorageDeals : ActorMethodBase<4> {
    using Params = v3::market::PublishStorageDeals::Params;
    struct Result {
      std::vector<DealId> deals;
      RleBitset valid;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(PublishStorageDeals::Result, deals, valid)
}  // namespace fc::vm::actor::builtin::v6::market
