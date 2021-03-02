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
#include "primitives/types.hpp"
#include "vm/actor/builtin/states/state.hpp"
#include "vm/actor/builtin/types/market/deal.hpp"

namespace fc::vm::actor::builtin::states {
  using adt::BalanceTable;
  using adt::CidKeyer;
  using adt::UvarintKeyer;
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::TokenAmount;
  using types::market::DealProposal;
  using types::market::DealState;

  struct MarketActorState : State {
    explicit MarketActorState(ActorVersion version)
        : State(ActorType::kMarket, version) {}

    using DealSet = adt::Set<UvarintKeyer>;

    adt::Array<DealProposal> proposals;
    adt::Array<DealState> states;
    adt::Map<DealProposal, CidKeyer> pending_proposals;
    BalanceTable escrow_table;
    BalanceTable locked_table;
    DealId next_deal{0};
    adt::Map<DealSet, UvarintKeyer> deals_by_epoch;
    ChainEpoch last_cron{-1};
    TokenAmount total_client_locked_collateral{};
    TokenAmount total_provider_locked_collateral{};
    TokenAmount total_client_storage_fee{};
  };

  using MarketActorStatePtr = std::shared_ptr<MarketActorState>;
}  // namespace fc::vm::actor::builtin::states
