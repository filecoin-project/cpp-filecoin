/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/states/market/v3/market_actor_state.hpp"

#include "vm/actor/builtin/types/market/policy.hpp"
#include "vm/runtime/runtime.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::actor::builtin::v3::market {
  using primitives::kChainEpochUndefined;
  using runtime::Runtime;
  using toolchain::Toolchain;
  using namespace types::market;

  outcome::result<void> MarketActorState::processDealExpired(
      const Runtime &runtime,
      const DealProposal &deal,
      const DealState &deal_state) {
    OUTCOME_TRY(runtime.requireState(deal_state.sector_start_epoch
                                     != kChainEpochUndefined));

    REQUIRE_NO_ERROR(unlockBalance(runtime,
                                   deal.provider,
                                   deal.provider_collateral,
                                   BalanceLockingReason::kProviderCollateral),
                     VMExitCode::kErrIllegalState);
    REQUIRE_NO_ERROR(unlockBalance(runtime,
                                   deal.client,
                                   deal.client_collateral,
                                   BalanceLockingReason::kClientCollateral),
                     VMExitCode::kErrIllegalState);

    return outcome::success();
  }

  outcome::result<std::tuple<TokenAmount, ChainEpoch, bool>>
  MarketActorState::updatePendingDealState(Runtime &runtime,
                                           DealId deal_id,
                                           const DealProposal &deal,
                                           const DealState &deal_state,
                                           ChainEpoch epoch) {
    TokenAmount slashed_sum{0};

    const auto updated{deal_state.last_updated_epoch != kChainEpochUndefined};
    const auto slashed{deal_state.slash_epoch != kChainEpochUndefined};

    OUTCOME_TRY(runtime.requireState(
        !updated || (deal_state.last_updated_epoch <= epoch)));

    if (deal.start_epoch > epoch) {
      return std::make_tuple(slashed_sum, kChainEpochUndefined, false);
    }

    auto payment_end_epoch = deal.end_epoch;
    if (slashed) {
      OUTCOME_TRY(runtime.requireState(epoch >= deal_state.slash_epoch));
      OUTCOME_TRY(
          runtime.requireState(deal_state.slash_epoch <= deal.end_epoch));
      payment_end_epoch = deal_state.slash_epoch;
    } else if (epoch < payment_end_epoch) {
      payment_end_epoch = epoch;
    }

    auto payment_start_epoch = deal.start_epoch;
    if (updated && (deal_state.last_updated_epoch > payment_start_epoch)) {
      payment_start_epoch = deal_state.last_updated_epoch;
    }

    const auto epochs_elapsed = payment_end_epoch - payment_start_epoch;
    const TokenAmount total_payment =
        epochs_elapsed * deal.storage_price_per_epoch;

    if (total_payment > 0) {
      REQUIRE_NO_ERROR(
          transferBalance(runtime, deal.client, deal.provider, total_payment),
          VMExitCode::kErrIllegalState);
    }

    const auto utils = Toolchain::createMarketUtils(runtime);

    if (slashed) {
      REQUIRE_NO_ERROR_A(
          remaining,
          utils->dealGetPaymentRemaining(deal, deal_state.slash_epoch),
          VMExitCode::kErrIllegalState);

      REQUIRE_NO_ERROR(unlockBalance(runtime,
                                     deal.client,
                                     remaining,
                                     BalanceLockingReason::kClientStorageFee),
                       VMExitCode::kErrIllegalState);

      REQUIRE_NO_ERROR(unlockBalance(runtime,
                                     deal.client,
                                     deal.client_collateral,
                                     BalanceLockingReason::kClientCollateral),
                       VMExitCode::kErrIllegalState);

      slashed_sum = deal.provider_collateral;

      REQUIRE_NO_ERROR(slashBalance(runtime,
                                    deal.provider,
                                    slashed_sum,
                                    BalanceLockingReason::kProviderCollateral),
                       VMExitCode::kErrIllegalState);

      return std::make_tuple(slashed_sum, kChainEpochUndefined, true);
    }

    if (epoch >= deal.end_epoch) {
      OUTCOME_TRY(processDealExpired(runtime, deal, deal_state));
      return std::make_tuple(slashed_sum, kChainEpochUndefined, true);
    }

    const auto next_epoch = epoch + kDealUpdatesInterval;

    return std::make_tuple(slashed_sum, next_epoch, false);
  }

}  // namespace fc::vm::actor::builtin::v3::market
