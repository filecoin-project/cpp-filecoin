/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/market/market_actor_state.hpp"

#include "storage/ipfs/datastore.hpp"

namespace fc::vm::actor::builtin::v2::market {
  outcome::result<Buffer> MarketActorState::toCbor() const {
    return Ipld::encode(*this);
  }
}  // namespace fc::vm::actor::builtin::v2::market
