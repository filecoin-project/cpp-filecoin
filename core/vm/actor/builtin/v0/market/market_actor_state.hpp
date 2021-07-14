/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/states/market_actor_state.hpp"

#include "codec/cbor/streams_annotation.hpp"

namespace fc::vm::actor::builtin::v0::market {

  struct MarketActorState : states::MarketActorState {};
  CBOR_TUPLE(MarketActorState,
             proposals,
             states,
             pending_proposals,
             escrow_table,
             locked_table,
             next_deal,
             deals_by_epoch,
             last_cron,
             total_client_locked_collateral,
             total_provider_locked_collateral,
             total_client_storage_fee)

}  // namespace fc::vm::actor::builtin::v0::market

namespace fc::cbor_blake {
  template <>
  struct CbVisitT<vm::actor::builtin::v0::market::MarketActorState> {
    template <typename Visitor>
    static void call(vm::actor::builtin::v0::market::MarketActorState &state,
                     const Visitor &visit) {
      visit(state.proposals);
      visit(state.states);
      visit(state.pending_proposals);
      visit(state.escrow_table);
      visit(state.locked_table);
      visit(state.deals_by_epoch);
    }
  };
}  // namespace fc::cbor_blake
