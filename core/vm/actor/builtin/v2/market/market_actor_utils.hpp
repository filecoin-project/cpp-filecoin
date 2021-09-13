/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v0/market/market_actor_utils.hpp"

namespace fc::vm::actor::builtin::v2::market {
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::DealWeight;
  using primitives::StoragePower;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using runtime::Runtime;
  using states::MarketActorStatePtr;
  using types::Controls;
  using types::market::ClientDealProposal;
  using types::market::DealProposal;

  class MarketUtils : public v0::market::MarketUtils {
   public:
    explicit MarketUtils(Runtime &r) : v0::market::MarketUtils(r) {}

    outcome::result<void> checkWithdrawCaller() const override;

    outcome::result<void> validateDeal(
        const ClientDealProposal &client_deal,
        const StoragePower &baseline_power,
        const StoragePower &network_raw_power,
        const StoragePower &network_qa_power) const override;

    outcome::result<std::tuple<DealWeight, DealWeight, uint64_t>>
    validateDealsForActivation(MarketActorStatePtr &state,
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

}  // namespace fc::vm::actor::builtin::v2::market
