/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "primitives/address/address.hpp"
#include "primitives/types.hpp"
#include "vm/actor/builtin/states/market_actor_state.hpp"
#include "vm/actor/builtin/v0/market/types.hpp"
#include "vm/exit_code/exit_code.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::utils {
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::DealWeight;
  using primitives::StoragePower;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using runtime::Runtime;
  using states::market::DealProposal;
  using states::market::DealState;
  using v0::market::BalanceLockingReason;
  using v0::market::ClientDealProposal;

  class MarketUtils {
   public:
    explicit MarketUtils(Runtime &r) : runtime(r) {}
    virtual ~MarketUtils() = default;

    virtual outcome::result<std::tuple<Address, Address, std::vector<Address>>>
    escrowAddress(const Address &address) const = 0;

    virtual outcome::result<void> unlockBalance(
        states::MarketActorStatePtr state,
        const Address &address,
        const TokenAmount &amount,
        BalanceLockingReason lock_reason) const = 0;

    virtual outcome::result<void> slashBalance(
        states::MarketActorStatePtr state,
        const Address &address,
        const TokenAmount &amount,
        BalanceLockingReason reason) const = 0;

    virtual outcome::result<void> transferBalance(
        states::MarketActorStatePtr state,
        const Address &from,
        const Address &to,
        const TokenAmount &amount) const = 0;

    virtual outcome::result<TokenAmount> processDealInitTimedOut(
        states::MarketActorStatePtr state, const DealProposal &deal) const = 0;

    virtual outcome::result<void> processDealExpired(
        states::MarketActorStatePtr state,
        const DealProposal &deal,
        const DealState &deal_state) const = 0;

    virtual outcome::result<void> dealProposalIsInternallyValid(
        const ClientDealProposal &client_deal) const = 0;

    virtual outcome::result<TokenAmount> dealGetPaymentRemaining(
        const DealProposal &deal, ChainEpoch slash_epoch) const = 0;

    virtual outcome::result<std::tuple<TokenAmount, ChainEpoch, bool>>
    updatePendingDealState(states::MarketActorStatePtr state,
                           DealId deal_id,
                           const DealProposal &deal,
                           const DealState &deal_state,
                           ChainEpoch epoch) const = 0;

    virtual outcome::result<void> maybeLockBalance(
        states::MarketActorStatePtr state,
        const Address &address,
        const TokenAmount &amount) const = 0;

    virtual outcome::result<void> lockClientAndProviderBalances(
        states::MarketActorStatePtr state, const DealProposal &deal) const = 0;

    virtual outcome::result<ChainEpoch> genRandNextEpoch(
        const DealProposal &deal) const = 0;

    virtual outcome::result<void> deleteDealProposalAndState(
        states::MarketActorStatePtr state,
        DealId deal_id,
        bool remove_proposal,
        bool remove_state) const = 0;

    virtual outcome::result<void> validateDealCanActivate(
        const DealProposal &deal,
        const Address &miner,
        const ChainEpoch &sector_expiration,
        const ChainEpoch &current_epoch) const = 0;

    virtual outcome::result<void> validateDeal(
        const ClientDealProposal &client_deal,
        const StoragePower &baseline_power,
        const StoragePower &network_raw_power,
        const StoragePower &network_qa_power) const = 0;

    virtual outcome::result<std::tuple<DealWeight, DealWeight>>
    validateDealsForActivation(states::MarketActorStatePtr state,
                               const std::vector<DealId> &deals,
                               const ChainEpoch &sector_expiry) const = 0;

   protected:
    Runtime &runtime;
  };

  using MarketUtilsPtr = std::shared_ptr<MarketUtils>;

}  // namespace fc::vm::actor::builtin::utils
