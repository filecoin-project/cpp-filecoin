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
  using states::DealArray;
  using states::MarketActorStatePtr;
  using types::Controls;
  using types::market::ClientDealProposal;
  using types::market::DealProposal;

  class MarketUtils : public utils::MarketUtils {
   public:
    explicit MarketUtils(Runtime &r) : utils::MarketUtils(r) {}

    outcome::result<void> checkWithdrawCaller() const override;

    outcome::result<void> assertCondition(bool condition) const override;

    outcome::result<void> checkCallers(const Address &provider) const override;

    outcome::result<std::tuple<Address, Address, std::vector<Address>>>
    escrowAddress(const Address &address) const override;

    outcome::result<void> dealProposalIsInternallyValid(
        const ClientDealProposal &client_deal) const override;

    outcome::result<TokenAmount> dealGetPaymentRemaining(
        const DealProposal &deal, ChainEpoch slash_epoch) const override;

    outcome::result<ChainEpoch> genRandNextEpoch(
        const DealProposal &deal) const override;

    outcome::result<void> deleteDealProposalAndState(
        MarketActorStatePtr &state,
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

    outcome::result<std::tuple<DealWeight, DealWeight, uint64_t>>
    validateDealsForActivation(MarketActorStatePtr &state,
                               const std::vector<DealId> &deals,
                               const ChainEpoch &sector_expiry) const override;

    outcome::result<std::tuple<DealWeight, DealWeight, uint64_t>>
    validateAndComputeDealWeight(
        DealArray &proposals,
        const std::vector<DealId> &deals,
        const ChainEpoch &sector_expiry) const override;

    outcome::result<StoragePower> getBaselinePowerFromRewardActor()
        const override;

    outcome::result<std::tuple<StoragePower, StoragePower>>
    getRawAndQaPowerFromPowerActor() const override;

    outcome::result<void> callVerifRegUseBytes(
        const DealProposal &deal) const override;

    outcome::result<void> callVerifRegRestoreBytes(
        const DealProposal &deal) const override;

    outcome::result<Controls> requestMinerControlAddress(
        const Address &miner) const override;
  };

}  // namespace fc::vm::actor::builtin::v0::market
