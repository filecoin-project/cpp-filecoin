/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/market/actor.hpp"

#include "vm/actor/builtin/market/policy.hpp"
#include "vm/actor/builtin/shared/shared.hpp"

namespace fc::vm::actor::builtin::market {
  using primitives::kChainEpochUndefined;
  using primitives::piece::PieceInfo;

  void State::load(std::shared_ptr<Ipld> ipld) {
    proposals.load(ipld);
    states.load(ipld);
    escrow_table.load(ipld);
    locked_table.load(ipld);
    deals_by_party.load(ipld);
  }

  outcome::result<void> State::flush() {
    OUTCOME_TRY(proposals.flush());
    OUTCOME_TRY(states.flush());
    OUTCOME_TRY(escrow_table.flush());
    OUTCOME_TRY(locked_table.flush());
    OUTCOME_TRY(deals_by_party.flush());
    return outcome::success();
  }

  outcome::result<DealState> State::getState(DealId deal_id) {
    OUTCOME_TRY(state, states.tryGet(deal_id));
    if (state) {
      return *state;
    }
    return DealState{
        kChainEpochUndefined, kChainEpochUndefined, kChainEpochUndefined};
  }

  outcome::result<void> State::addDeal(DealId deal_id,
                                       const DealProposal &deal) {
    OUTCOME_TRY(proposals.set(deal_id, deal));
    for (auto address : {&deal.provider, &deal.client}) {
      OUTCOME_TRY(set, deals_by_party.tryGet(*address));
      if (!set) {
        set = State::PartyDeals{};
      }
      set->load(deals_by_party.hamt.getIpld());
      OUTCOME_TRY(set->set(deal_id, {}));
      OUTCOME_TRY(set->flush());
      OUTCOME_TRY(deals_by_party.set(*address, *set));
    }
    return outcome::success();
  }

  TokenAmount clientPayment(ChainEpoch epoch,
                            const DealProposal &deal,
                            const DealState &deal_state) {
    auto end = std::min(epoch, deal.end_epoch);
    if (deal_state.slash_epoch != kChainEpochUndefined) {
      end = std::min(end, deal_state.slash_epoch);
    }
    auto start = deal.start_epoch;
    if (deal_state.last_updated_epoch != kChainEpochUndefined) {
      start = std::max(start, deal_state.last_updated_epoch);
    }
    return deal.storage_price_per_epoch * (end - start);
  }

  outcome::result<State> loadState(Runtime &runtime) {
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    state.load(runtime);
    return std::move(state);
  }

  outcome::result<void> commitState(Runtime &runtime, State &state) {
    OUTCOME_TRY(state.flush());
    return runtime.commitState(state);
  }

  outcome::result<void> assertCallerIsMiner(Runtime &runtime) {
    OUTCOME_TRY(code, runtime.getActorCodeID(runtime.getImmediateCaller()));
    if (code != kStorageMinerCodeCid) {
      return VMExitCode::MARKET_ACTOR_WRONG_CALLER;
    }
    return outcome::success();
  }

  outcome::result<void> assertCallerIsSignable(Runtime &runtime) {
    OUTCOME_TRY(code, runtime.getActorCodeID(runtime.getImmediateCaller()));
    if (!isSignableActor(code)) {
      return VMExitCode::MARKET_ACTOR_WRONG_CALLER;
    }
    return outcome::success();
  }

  outcome::result<std::pair<Address, Address>> escrowAddress(
      Runtime &runtime, const Address &address) {
    OUTCOME_TRY(nominal, runtime.resolveAddress(address));
    OUTCOME_TRY(code, runtime.getActorCodeID(nominal));
    if (code == kStorageMinerCodeCid) {
      OUTCOME_TRY(miner, requestMinerControlAddress(runtime, nominal));
      if (runtime.getImmediateCaller() != miner.worker
          && runtime.getImmediateCaller() != miner.owner) {
        return VMExitCode::MARKET_ACTOR_WRONG_CALLER;
      }
      return std::make_pair(nominal, miner.owner);
    }
    if (!isSignableActor(code)) {
      return VMExitCode::MARKET_ACTOR_WRONG_CALLER;
    }
    return std::make_pair(nominal, nominal);
  }

  outcome::result<void> unlockBalance(State &state,
                                      const Address &address,
                                      TokenAmount amount) {
    if (amount < 0) {
      return VMExitCode::MARKET_ACTOR_ILLEGAL_STATE;
    }
    return state.locked_table.subtract(address, amount);
  }

  outcome::result<void> slashBalance(State &state,
                                     const Address &address,
                                     TokenAmount amount) {
    if (amount < 0) {
      return VMExitCode::MARKET_ACTOR_ILLEGAL_STATE;
    }
    OUTCOME_TRY(state.escrow_table.subtract(address, amount));
    OUTCOME_TRY(state.locked_table.subtract(address, amount));
    return outcome::success();
  }

  outcome::result<void> removeDeal(State &state,
                                   DealId deal_id,
                                   const DealProposal &deal) {
    OUTCOME_TRY(state.proposals.remove(deal_id));
    for (auto address : {&deal.provider, &deal.client}) {
      OUTCOME_TRY(set, state.deals_by_party.get(*address));
      set.load(state.deals_by_party.hamt.getIpld());
      OUTCOME_TRY(set.remove(deal_id));
      OUTCOME_TRY(set.flush());
      OUTCOME_TRY(state.deals_by_party.set(*address, set));
    }
    return outcome::success();
  }

  outcome::result<void> transferBalance(State &state,
                                        const Address &from,
                                        const Address &to,
                                        TokenAmount amount) {
    if (amount < 0) {
      return VMExitCode::MARKET_ACTOR_ILLEGAL_STATE;
    }
    OUTCOME_TRY(state.escrow_table.subtract(from, amount));
    OUTCOME_TRY(state.locked_table.subtract(from, amount));
    OUTCOME_TRY(state.escrow_table.add(to, amount));
    return outcome::success();
  }

  outcome::result<TokenAmount> updatePendingDealStates(
      Runtime &runtime,
      State &state,
      const std::vector<DealId> &deals,
      ChainEpoch epoch) {
    TokenAmount slashed_total;
    for (auto deal_id : deals) {
      OUTCOME_TRY(deal, state.proposals.get(deal_id));
      OUTCOME_TRY(deal_state, state.getState(deal_id));
      if (deal_state.last_updated_epoch == epoch) {
        continue;
      }
      if (deal_state.sector_start_epoch == kChainEpochUndefined) {
        if (epoch > deal.start_epoch) {
          OUTCOME_TRY(unlockBalance(
              state, deal.client, deal.clientBalanceRequirement()));
          auto slashed = collateralPenaltyForDealActivationMissed(
              deal.provider_collateral);
          OUTCOME_TRY(slashBalance(state, deal.provider, slashed));
          slashed_total += slashed;
          OUTCOME_TRY(
              unlockBalance(state,
                            deal.provider,
                            deal.providerBalanceRequirement() - slashed));
          OUTCOME_TRY(removeDeal(state, deal_id, deal));
        }
        continue;
      }
      OUTCOME_TRY(transferBalance(state,
                                  deal.client,
                                  deal.provider,
                                  clientPayment(epoch, deal, deal_state)));
      if (deal_state.slash_epoch != kChainEpochUndefined) {
        OUTCOME_TRY(unlockBalance(
            state,
            deal.client,
            deal.client_collateral
                + deal.storage_price_per_epoch
                      * (deal.end_epoch - deal_state.slash_epoch)));
        auto slashed = deal.provider_collateral;
        OUTCOME_TRY(slashBalance(state, deal.provider, slashed));
        slashed_total += slashed;
        OUTCOME_TRY(removeDeal(state, deal_id, deal));
        continue;
      }
      if (epoch >= deal.end_epoch) {
        OUTCOME_TRY(unlockBalance(state, deal.client, deal.client_collateral));
        OUTCOME_TRY(
            unlockBalance(state, deal.provider, deal.provider_collateral));
        OUTCOME_TRY(removeDeal(state, deal_id, deal));
        continue;
      }
      deal_state.last_updated_epoch = epoch;
      OUTCOME_TRY(state.states.set(deal_id, deal_state));
    }
    return slashed_total;
  }

  outcome::result<TokenAmount> updatePendingDealStatesForParty(
      Runtime &runtime, State &state, const Address &address) {
    std::vector<DealId> deals;
    OUTCOME_TRY(set, state.deals_by_party.tryGet(address));
    if (set) {
      set->load(runtime);
      OUTCOME_TRY(
          set->visit([&](auto &deal_id, auto &) -> outcome::result<void> {
            deals.emplace_back(deal_id);
            return outcome::success();
          }));
    }
    return updatePendingDealStates(
        runtime, state, deals, runtime.getCurrentEpoch() - 1);
  }

  outcome::result<void> maybeLockBalance(State &state,
                                         const Address &address,
                                         TokenAmount amount) {
    OUTCOME_TRY(escrow, state.escrow_table.get(address));
    OUTCOME_TRY(locked, state.locked_table.get(address));
    if (locked + amount > escrow) {
      return VMExitCode::MARKET_ACTOR_INSUFFICIENT_FUNDS;
    }
    OUTCOME_TRY(state.locked_table.add(address, amount));
    return outcome::success();
  }

  outcome::result<void> validateDeal(Runtime &runtime,
                                     const ClientDealProposal &proposal) {
    auto &deal = proposal.proposal;
    auto duration = deal.duration();
    if (duration <= 0) {
      return VMExitCode::MARKET_ACTOR_ILLEGAL_ARGUMENT;
    }
    OUTCOME_TRY(encoded, codec::cbor::encode(deal));
    OUTCOME_TRY(verified,
                runtime.verifySignature(
                    proposal.client_signature, deal.client, encoded));
    if (!verified || runtime.getCurrentEpoch() > deal.start_epoch
        || !dealDurationBounds(deal.piece_size).in(duration)
        || !dealPricePerEpochBounds(deal.piece_size, duration)
                .in(deal.storage_price_per_epoch)
        || !dealProviderCollateralBounds(deal.piece_size, duration)
                .in(deal.provider_collateral)
        || !dealClientCollateralBounds(deal.piece_size, duration)
                .in(deal.client_collateral)) {
      return VMExitCode::MARKET_ACTOR_ILLEGAL_ARGUMENT;
    }
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(Construct) {
    if (runtime.getImmediateCaller() != kInitAddress) {
      return VMExitCode::MARKET_ACTOR_WRONG_CALLER;
    }
    State state{
        .proposals = {},
        .states = {},
        .escrow_table = {},
        .locked_table = {},
        .next_deal = 0,
        .deals_by_party = {},
    };
    state.load(runtime);
    OUTCOME_TRY(commitState(runtime, state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(AddBalance) {
    OUTCOME_TRY(addresses, escrowAddress(runtime, params));
    OUTCOME_TRY(state, loadState(runtime));
    OUTCOME_TRY(state.escrow_table.addCreate(addresses.first,
                                             runtime.getValueReceived()));
    OUTCOME_TRY(state.locked_table.addCreate(addresses.first, 0));
    OUTCOME_TRY(commitState(runtime, state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(WithdrawBalance) {
    if (params.amount < 0) {
      return VMExitCode::MARKET_ACTOR_ILLEGAL_ARGUMENT;
    }
    OUTCOME_TRY(addresses, escrowAddress(runtime, params.address));
    OUTCOME_TRY(state, loadState(runtime));
    OUTCOME_TRY(
        slashed,
        updatePendingDealStatesForParty(runtime, state, addresses.first));
    OUTCOME_TRY(min, state.locked_table.get(addresses.first));
    OUTCOME_TRY(extracted,
                state.escrow_table.subtractWithMin(
                    addresses.first, params.amount, min));
    OUTCOME_TRY(commitState(runtime, state));
    if (slashed > 0) {
      OUTCOME_TRY(runtime.sendFunds(kBurntFundsActorAddress, slashed));
    }
    OUTCOME_TRY(runtime.sendFunds(addresses.second, extracted));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(HandleExpiredDeals) {
    OUTCOME_TRY(assertCallerIsSignable(runtime));
    OUTCOME_TRY(state, loadState(runtime));
    OUTCOME_TRY(slashed,
                updatePendingDealStates(
                    runtime, state, params.deals, runtime.getCurrentEpoch()));
    OUTCOME_TRY(commitState(runtime, state));
    OUTCOME_TRY(runtime.sendFunds(kBurntFundsActorAddress, slashed));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(PublishStorageDeals) {
    OUTCOME_TRY(assertCallerIsSignable(runtime));
    if (params.deals.empty()) {
      return VMExitCode::MARKET_ACTOR_ILLEGAL_ARGUMENT;
    }
    auto providerRaw = params.deals[0].proposal.provider;
    OUTCOME_TRY(provider, runtime.resolveAddress(providerRaw));
    OUTCOME_TRY(addresses, requestMinerControlAddress(runtime, provider));
    if (addresses.worker != runtime.getImmediateCaller()) {
      return VMExitCode::MARKET_ACTOR_FORBIDDEN;
    }
    TokenAmount slashed_total;
    std::vector<DealId> deals;
    OUTCOME_TRY(state, loadState(runtime));
    for (auto proposal : params.deals) {
      OUTCOME_TRY(validateDeal(runtime, proposal));

      auto &deal = proposal.proposal;
      if (deal.provider != provider && deal.provider != providerRaw) {
        return VMExitCode::MARKET_ACTOR_ILLEGAL_ARGUMENT;
      }

      OUTCOME_TRY(client, runtime.resolveAddress(deal.client));
      deal.provider = provider;
      deal.client = client;

      OUTCOME_TRY(slashed_from_client_deals,
                  updatePendingDealStatesForParty(runtime, state, client));
      OUTCOME_TRY(slashed_from_provider_deals,
                  updatePendingDealStatesForParty(runtime, state, provider));
      slashed_total += slashed_from_client_deals + slashed_from_provider_deals;

      OUTCOME_TRY(
          maybeLockBalance(state, client, deal.clientBalanceRequirement()));
      OUTCOME_TRY(
          maybeLockBalance(state, provider, deal.providerBalanceRequirement()));

      auto deal_id = state.next_deal++;
      OUTCOME_TRY(state.addDeal(deal_id, deal));
      deals.emplace_back(deal_id);
    }
    OUTCOME_TRY(commitState(runtime, state));
    OUTCOME_TRY(runtime.sendFunds(kBurntFundsActorAddress, slashed_total));
    return Result{.deals = deals};
  }

  ACTOR_METHOD_IMPL(VerifyDealsOnSectorProveCommit) {
    OUTCOME_TRY(assertCallerIsMiner(runtime));
    auto miner = runtime.getImmediateCaller();
    OUTCOME_TRY(state, loadState(runtime));
    DealWeight weight_total;
    for (auto deal_id : params.deals) {
      OUTCOME_TRY(deal, state.proposals.get(deal_id));
      OUTCOME_TRY(deal_state, state.getState(deal_id));
      if (deal.provider != miner
          || deal_state.sector_start_epoch != kChainEpochUndefined
          || runtime.getCurrentEpoch() > deal.start_epoch
          || deal.end_epoch > params.sector_expiry) {
        return outcome::failure(VMExitCode::MARKET_ACTOR_ILLEGAL_ARGUMENT);
      }
      deal_state.sector_start_epoch = runtime.getCurrentEpoch();
      OUTCOME_TRY(state.states.set(deal_id, deal_state));
      weight_total += deal.piece_size * deal.duration();
    }
    OUTCOME_TRY(commitState(runtime, state));
    return weight_total;
  }

  ACTOR_METHOD_IMPL(OnMinerSectorsTerminate) {
    OUTCOME_TRY(assertCallerIsMiner(runtime));
    auto miner = runtime.getImmediateCaller();
    OUTCOME_TRY(state, loadState(runtime));
    for (auto deal_id : params.deals) {
      OUTCOME_TRY(deal, state.proposals.get(deal_id));
      if (deal.provider != miner) {
        return VMExitCode::MARKET_ACTOR_FORBIDDEN;
      }
      OUTCOME_TRY(deal_state, state.getState(deal_id));
      deal_state.slash_epoch = runtime.getCurrentEpoch();
      OUTCOME_TRY(state.states.set(deal_id, deal_state));
    }
    OUTCOME_TRY(commitState(runtime, state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(ComputeDataCommitment) {
    OUTCOME_TRY(assertCallerIsMiner(runtime));
    OUTCOME_TRY(state, loadState(runtime));
    std::vector<PieceInfo> pieces;
    for (auto deal_id : params.deals) {
      OUTCOME_TRY(deal, state.proposals.get(deal_id));
      pieces.emplace_back(PieceInfo{deal.piece_size, deal.piece_cid});
    }
    return runtime.computeUnsealedSectorCid(params.sector_type, pieces);
  }

  ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<AddBalance>(),
      exportMethod<WithdrawBalance>(),
      exportMethod<HandleExpiredDeals>(),
      exportMethod<PublishStorageDeals>(),
      exportMethod<VerifyDealsOnSectorProveCommit>(),
      exportMethod<OnMinerSectorsTerminate>(),
      exportMethod<ComputeDataCommitment>(),
  };
}  // namespace fc::vm::actor::builtin::market
