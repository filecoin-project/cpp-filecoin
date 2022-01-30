/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/states/market/v0/market_actor_state.hpp"

namespace fc::vm::actor::builtin::v3::market {
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::TokenAmount;
  using runtime::Runtime;
  using types::market::DealProposal;
  using types::market::DealState;

  struct MarketActorState : v0::market::MarketActorState {
    outcome::result<std::tuple<TokenAmount, ChainEpoch, bool>>
    updatePendingDealState(Runtime &runtime,
                           DealId deal_id,
                           const DealProposal &deal,
                           const DealState &deal_state,
                           ChainEpoch epoch) override;

   protected:
    outcome::result<void> check(bool condition) const override;
  };
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

}  // namespace fc::vm::actor::builtin::v3::market

namespace fc::cbor_blake {
  template <>
  struct CbVisitT<vm::actor::builtin::v3::market::MarketActorState> {
    template <typename Visitor>
    static void call(vm::actor::builtin::v3::market::MarketActorState &state,
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
