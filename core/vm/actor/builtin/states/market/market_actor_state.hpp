/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "adt/array.hpp"
#include "adt/balance_table.hpp"
#include "adt/cid_key.hpp"
#include "adt/set.hpp"
#include "adt/uvarint_key.hpp"
#include "primitives/address/address.hpp"
#include "primitives/types.hpp"
#include "vm/actor/builtin/types/market/deal.hpp"
#include "vm/actor/builtin/types/universal/universal.hpp"

// Forward declaration
namespace fc::vm::runtime {
  class Runtime;
}

namespace fc::vm::actor::builtin::states {
  using adt::BalanceTable;
  using adt::CidKeyer;
  using adt::UvarintKeyer;
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using types::market::BalanceLockingReason;
  using types::market::DealProposal;
  using types::market::DealState;

  constexpr size_t kProposalsAmtBitwidth = 5;
  constexpr size_t kStatesAmtBitwidth = 6;

  using DealArray = adt::Array<DealProposal, kProposalsAmtBitwidth>;
  using DealSet = adt::Set<UvarintKeyer>;

  struct MarketActorState {
    virtual ~MarketActorState() = default;

    DealArray proposals;
    adt::Array<DealState, kStatesAmtBitwidth> states;
    adt::Map<DealProposal, CidKeyer> pending_proposals_0;
    adt::Set<CidKeyer> pending_proposals_3;
    BalanceTable escrow_table;
    BalanceTable locked_table;
    DealId next_deal{0};
    adt::Map<DealSet, UvarintKeyer> deals_by_epoch;
    ChainEpoch last_cron{-1};
    TokenAmount total_client_locked_collateral{};
    TokenAmount total_provider_locked_collateral{};
    TokenAmount total_client_storage_fee{};

    // Methods
    outcome::result<void> unlockBalance(const runtime::Runtime &runtime,
                                        const Address &address,
                                        const TokenAmount &amount,
                                        BalanceLockingReason lock_reason);

    outcome::result<void> slashBalance(const runtime::Runtime &runtime,
                                       const Address &address,
                                       const TokenAmount &amount,
                                       BalanceLockingReason reason);

    outcome::result<void> transferBalance(const runtime::Runtime &runtime,
                                          const Address &from,
                                          const Address &to,
                                          const TokenAmount &amount);

    outcome::result<TokenAmount> processDealInitTimedOut(
        const runtime::Runtime &runtime, const DealProposal &deal);

    virtual outcome::result<void> processDealExpired(const runtime::Runtime &runtime,
                                             const DealProposal &deal,
                                             const DealState &deal_state);

    virtual outcome::result<std::tuple<TokenAmount, ChainEpoch, bool>>
    updatePendingDealState(runtime::Runtime &runtime,
                           DealId deal_id,
                           const DealProposal &deal,
                           const DealState &deal_state,
                           ChainEpoch epoch);

    outcome::result<void> maybeLockBalance(const runtime::Runtime &runtime,
                                           const Address &address,
                                           const TokenAmount &amount);

    outcome::result<void> lockClientAndProviderBalances(
        const runtime::Runtime &runtime, const DealProposal &deal);
  };

  using MarketActorStatePtr = types::Universal<MarketActorState>;

}  // namespace fc::vm::actor::builtin::states
