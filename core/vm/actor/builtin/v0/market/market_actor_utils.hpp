/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/utils/market_actor_utils.hpp"

namespace fc::vm::actor::builtin::v0::market {
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::DealWeight;
  using primitives::StoragePower;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using runtime::Runtime;
  using states::MarketActorStatePtr;
  using types::market::BalanceLockingReason;
  using types::market::ClientDealProposal;
  using types::market::DealProposal;
  using types::market::DealState;

  class MarketUtils : public utils::MarketUtils {
   public:
    explicit MarketUtils(Runtime &r) : utils::MarketUtils(r) {}

    outcome::result<std::tuple<Address, Address, std::vector<Address>>>
    escrowAddress(const Address &address) const override;

    outcome::result<void> unlockBalance(
        MarketActorStatePtr state,
        const Address &address,
        const TokenAmount &amount,
        BalanceLockingReason lock_reason) const override;

    outcome::result<void> slashBalance(
        MarketActorStatePtr state,
        const Address &address,
        const TokenAmount &amount,
        BalanceLockingReason reason) const override;

    outcome::result<void> transferBalance(
        MarketActorStatePtr state,
        const Address &from,
        const Address &to,
        const TokenAmount &amount) const override;

    outcome::result<TokenAmount> processDealInitTimedOut(
        MarketActorStatePtr state, const DealProposal &deal) const override;

    outcome::result<void> processDealExpired(
        MarketActorStatePtr state,
        const DealProposal &deal,
        const DealState &deal_state) const override;

    outcome::result<void> dealProposalIsInternallyValid(
        const ClientDealProposal &client_deal) const override;

    outcome::result<TokenAmount> dealGetPaymentRemaining(
        const DealProposal &deal, ChainEpoch slash_epoch) const override;

    outcome::result<std::tuple<TokenAmount, ChainEpoch, bool>>
    updatePendingDealState(MarketActorStatePtr state,
                           DealId deal_id,
                           const DealProposal &deal,
                           const DealState &deal_state,
                           ChainEpoch epoch) const override;

    outcome::result<void> maybeLockBalance(
        MarketActorStatePtr state,
        const Address &address,
        const TokenAmount &amount) const override;

    outcome::result<void> lockClientAndProviderBalances(
        MarketActorStatePtr state, const DealProposal &deal) const override;

    outcome::result<ChainEpoch> genRandNextEpoch(
        const DealProposal &deal) const override;

    outcome::result<void> deleteDealProposalAndState(
        MarketActorStatePtr state,
        DealId deal_id,
        bool remove_proposal,
        bool remove_state) const override;

    outcome::result<void> validateDealCanActivate(
        const DealProposal &deal,
        const Address &miner,
        const ChainEpoch &sector_expiration,
        const ChainEpoch &current_epoch) const override;

    outcome::result<void> validateDeal(
        const ClientDealProposal &client_deal,
        const StoragePower &baseline_power,
        const StoragePower &network_raw_power,
        const StoragePower &network_qa_power) const override;

    outcome::result<std::tuple<DealWeight, DealWeight>>
    validateDealsForActivation(MarketActorStatePtr state,
                               const std::vector<DealId> &deals,
                               const ChainEpoch &sector_expiry) const override;
  };

}  // namespace fc::vm::actor::builtin::v0::market
