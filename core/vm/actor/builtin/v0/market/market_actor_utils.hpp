/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "primitives/address/address.hpp"
#include "primitives/types.hpp"
#include "vm/actor/builtin/v0/market/market_actor_state.hpp"
#include "vm/exit_code/exit_code.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::utils::market {
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::DealWeight;
  using primitives::StoragePower;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using runtime::Runtime;
  using v0::market::BalanceLockingReason;
  using v0::market::ClientDealProposal;
  using v0::market::DealProposal;
  using v0::market::DealState;
  using v0::market::State;

  outcome::result<std::tuple<Address, Address, std::vector<Address>>>
  escrowAddress(Runtime &runtime, const Address &address);

  outcome::result<void> unlockBalance(const Runtime &runtime,
                                      State &state,
                                      const Address &address,
                                      const TokenAmount &amount,
                                      BalanceLockingReason lock_reason);

  outcome::result<void> slashBalance(const Runtime &runtime,
                                     State &state,
                                     const Address &address,
                                     const TokenAmount &amount,
                                     BalanceLockingReason reason);

  outcome::result<void> transferBalance(const Runtime &runtime,
                                        State &state,
                                        const Address &from,
                                        const Address &to,
                                        const TokenAmount &amount);

  outcome::result<TokenAmount> processDealInitTimedOut(
      const Runtime &runtime, State &state, const DealProposal &deal);

  outcome::result<void> processDealExpired(Runtime &runtime,
                                           State &state,
                                           const DealProposal &deal,
                                           const DealState &deal_state);

  outcome::result<void> dealProposalIsInternallyValid(
      Runtime &runtime, const ClientDealProposal &client_deal);

  outcome::result<TokenAmount> dealGetPaymentRemaining(const Runtime &runtime,
                                                       const DealProposal &deal,
                                                       ChainEpoch slash_epoch);

  outcome::result<std::tuple<TokenAmount, ChainEpoch, bool>>
  updatePendingDealState(const Runtime &runtime,
                         State &state,
                         DealId deal_id,
                         const DealProposal &deal,
                         const DealState &deal_state,
                         ChainEpoch epoch);

  outcome::result<void> maybeLockBalance(const Runtime &runtime,
                                         State &state,
                                         const Address &address,
                                         const TokenAmount &amount);

  outcome::result<void> lockClientAndProviderBalances(const Runtime &runtime,
                                                      State &state,
                                                      const DealProposal &deal);

  outcome::result<ChainEpoch> genRandNextEpoch(const Runtime &runtime,
                                               const DealProposal &deal);

  outcome::result<void> deleteDealProposalAndState(State &state,
                                                   DealId deal_id,
                                                   bool remove_proposal,
                                                   bool remove_state);

  outcome::result<void> validateDealCanActivate(
      const DealProposal &deal,
      const Address &miner,
      const ChainEpoch &sector_expiration,
      const ChainEpoch &current_epoch);

  outcome::result<void> validateDeal(Runtime &runtime,
                                     const ClientDealProposal &client_deal,
                                     const StoragePower &baseline_power,
                                     const StoragePower &network_raw_power,
                                     const StoragePower &network_qa_power);

  outcome::result<std::tuple<DealWeight, DealWeight>>
  validateDealsForActivation(const Runtime &runtime,
                             State &state,
                             const std::vector<DealId> &deals,
                             const ChainEpoch &sector_expiry);

}  // namespace fc::vm::actor::builtin::utils::market
