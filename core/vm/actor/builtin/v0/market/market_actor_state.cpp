/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/market/market_actor_state.hpp"

#include "storage/ipfs/datastore.hpp"

namespace fc::vm::actor::builtin::v0::market {
  ACTOR_STATE_TO_CBOR_THIS(MarketActorState)
}  // namespace fc::vm::actor::builtin::v0::market
