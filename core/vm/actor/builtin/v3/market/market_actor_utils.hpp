/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v2/market/market_actor_utils.hpp"

namespace fc::vm::actor::builtin::v3::market {
  using primitives::ChainEpoch;
  using primitives::StoragePower;
  using primitives::address::Address;
  using runtime::Runtime;
  using types::Controls;
  using types::Universal;
  using types::market::DealProposal;

  class MarketUtils : public v2::market::MarketUtils {
   public:
    explicit MarketUtils(Runtime &r) : v2::market::MarketUtils(r) {}

    outcome::result<void> checkCallers(const Address &provider) const override;

    outcome::result<StoragePower> getBaselinePowerFromRewardActor()
        const override;

    outcome::result<std::tuple<StoragePower, StoragePower>>
    getRawAndQaPowerFromPowerActor() const override;

    outcome::result<void> callVerifRegUseBytes(
        const Universal<DealProposal> &deal) const override;

    outcome::result<void> callVerifRegRestoreBytes(
        const Universal<DealProposal> &deal) const override;

    outcome::result<Controls> requestMinerControlAddress(
        const Address &miner) const override;
  };

}  // namespace fc::vm::actor::builtin::v3::market
