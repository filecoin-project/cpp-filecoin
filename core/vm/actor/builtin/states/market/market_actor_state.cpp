/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/states/market/market_actor_state.hpp"

#include "vm/actor/builtin/types/market/policy.hpp"
#include "vm/runtime/runtime.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::actor::builtin::states {
  using primitives::kChainEpochUndefined;
  using runtime::Runtime;
  using toolchain::Toolchain;
  using namespace types::market;

  outcome::result<void> MarketActorState::unlockBalance(
      const Runtime &runtime,
      const Address &address,
      const TokenAmount &amount,
      BalanceLockingReason lock_reason) {
    VM_ASSERT(amount >= 0);

    OUTCOME_TRY(locked_table.subtract(address, amount));

    switch (lock_reason) {
      case BalanceLockingReason::kClientCollateral:
        total_client_locked_collateral -= amount;
        break;
      case BalanceLockingReason::kClientStorageFee:
        total_client_storage_fee -= amount;
        break;
      case BalanceLockingReason::kProviderCollateral:
        total_provider_locked_collateral -= amount;
        break;
    }
    return outcome::success();
  }

  outcome::result<void> MarketActorState::slashBalance(
      const Runtime &runtime,
      const Address &address,
      const TokenAmount &amount,
      BalanceLockingReason reason) {
    VM_ASSERT(amount >= 0);
    OUTCOME_TRY(escrow_table.subtract(address, amount));
    return unlockBalance(runtime, address, amount, reason);
  }

  outcome::result<void> MarketActorState::transferBalance(
      const Runtime &runtime,
      const Address &from,
      const Address &to,
      const TokenAmount &amount) {
    VM_ASSERT(amount >= 0);
    CHANGE_ERROR_ABORT(escrow_table.subtract(from, amount),
                       VMExitCode::kErrIllegalState);
    CHANGE_ERROR_ABORT(
        unlockBalance(
            runtime, from, amount, BalanceLockingReason::kClientStorageFee),
        VMExitCode::kErrIllegalState);
    CHANGE_ERROR_ABORT(escrow_table.add(to, amount),
                       VMExitCode::kErrIllegalState);
    return outcome::success();
  }

  outcome::result<TokenAmount> MarketActorState::processDealInitTimedOut(
      const Runtime &runtime, const DealProposal &deal) {
    CHANGE_ERROR_ABORT(unlockBalance(runtime,
                                     deal.client,
                                     deal.getTotalStorageFee(),
                                     BalanceLockingReason::kClientStorageFee),
                       VMExitCode::kErrIllegalState);
    CHANGE_ERROR_ABORT(unlockBalance(runtime,
                                     deal.client,
                                     deal.client_collateral,
                                     BalanceLockingReason::kClientCollateral),
                       VMExitCode::kErrIllegalState);

    const auto slashed =
        collateralPenaltyForDealActivationMissed(deal.provider_collateral);
    const auto amount_remaining = deal.providerBalanceRequirement() - slashed;

    CHANGE_ERROR_ABORT(slashBalance(runtime,
                                    deal.provider,
                                    slashed,
                                    BalanceLockingReason::kProviderCollateral),
                       VMExitCode::kErrIllegalState);
    CHANGE_ERROR_ABORT(unlockBalance(runtime,
                                     deal.provider,
                                     amount_remaining,
                                     BalanceLockingReason::kProviderCollateral),
                       VMExitCode::kErrIllegalState);

    return slashed;
  }

  outcome::result<void> MarketActorState::processDealExpired(
      const Runtime &runtime,
      const DealProposal &deal,
      const DealState &deal_state) {
    VM_ASSERT(deal_state.sector_start_epoch != kChainEpochUndefined);

    CHANGE_ERROR_ABORT(unlockBalance(runtime,
                                     deal.provider,
                                     deal.provider_collateral,
                                     BalanceLockingReason::kProviderCollateral),
                       VMExitCode::kErrIllegalState);
    CHANGE_ERROR_ABORT(unlockBalance(runtime,
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

    VM_ASSERT(!updated || (deal_state.last_updated_epoch <= epoch));

    if (deal.start_epoch > epoch) {
      return std::make_tuple(slashed_sum, kChainEpochUndefined, false);
    }

    auto payment_end_epoch = deal.end_epoch;
    if (slashed) {
      VM_ASSERT(epoch >= deal_state.slash_epoch);
      VM_ASSERT(deal_state.slash_epoch <= deal.end_epoch);
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
      OUTCOME_TRY(
          transferBalance(runtime, deal.client, deal.provider, total_payment));
    }

    const auto utils = Toolchain::createMarketUtils(runtime);

    if (slashed) {
      OUTCOME_TRY(remaining,
                  utils->dealGetPaymentRemaining(deal, deal_state.slash_epoch));

      CHANGE_ERROR_ABORT(unlockBalance(runtime,
                                       deal.client,
                                       remaining,
                                       BalanceLockingReason::kClientStorageFee),
                         VMExitCode::kErrIllegalState);

      CHANGE_ERROR_ABORT(unlockBalance(runtime,
                                       deal.client,
                                       deal.client_collateral,
                                       BalanceLockingReason::kClientCollateral),
                         VMExitCode::kErrIllegalState);

      slashed_sum = deal.provider_collateral;

      CHANGE_ERROR_ABORT(
          slashBalance(runtime,
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

  outcome::result<void> MarketActorState::maybeLockBalance(
      const Runtime &runtime,
      const Address &address,
      const TokenAmount &amount) {
    VM_ASSERT(amount >= 0);

    CHANGE_ERROR_A(
        locked, locked_table.get(address), VMExitCode::kErrIllegalState);
    CHANGE_ERROR_A(
        escrow, escrow_table.get(address), VMExitCode::kErrIllegalState);

    if ((locked + amount) > escrow) {
      return VMExitCode::kErrInsufficientFunds;
    }

    CHANGE_ERROR(locked_table.add(address, amount),
                 VMExitCode::kErrIllegalState);

    return outcome::success();
  }

  outcome::result<void> MarketActorState::lockClientAndProviderBalances(
      const Runtime &runtime, const DealProposal &deal) {
    OUTCOME_TRY(maybeLockBalance(
        runtime, deal.client, deal.clientBalanceRequirement()));
    OUTCOME_TRY(maybeLockBalance(
        runtime, deal.provider, deal.providerBalanceRequirement()));
    total_client_locked_collateral += deal.client_collateral;
    total_client_storage_fee += deal.getTotalStorageFee();
    total_provider_locked_collateral += deal.provider_collateral;
    return outcome::success();
  }

}  // namespace fc::vm::actor::builtin::states
