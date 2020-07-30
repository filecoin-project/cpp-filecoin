/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/market/actor.hpp"

#include "crypto/hasher/hasher.hpp"
#include "vm/actor/builtin/market/policy.hpp"
#include "vm/actor/builtin/shared/shared.hpp"
#include "vm/actor/builtin/verified_registry/actor.hpp"

namespace fc::vm::actor::builtin::market {
  using crypto::Hasher;
  using primitives::kChainEpochUndefined;
  using primitives::piece::PieceInfo;

  CID ClientDealProposal::cid() const {
    OUTCOME_EXCEPT(bytes, codec::cbor::encode(*this));
    return {
        CID::Version::V1, CID::Multicodec::DAG_CBOR, Hasher::sha2_256(bytes)};
  }

  outcome::result<State> loadState(Runtime &runtime) {
    return runtime.getCurrentActorStateCbor<State>();
  }

  outcome::result<std::pair<Address, Address>> escrowAddress(
      Runtime &runtime, const Address &address) {
    OUTCOME_TRY(nominal, runtime.resolveAddress(address));
    OUTCOME_TRY(code, runtime.getActorCodeID(nominal));
    if (code == kStorageMinerCodeCid) {
      OUTCOME_TRY(miner, requestMinerControlAddress(runtime, nominal));
      if (runtime.getImmediateCaller() != miner.worker
          && runtime.getImmediateCaller() != miner.owner) {
        return VMExitCode::kMarketActorWrongCaller;
      }
      return std::make_pair(nominal, miner.owner);
    }
    OUTCOME_TRY(runtime.validateImmediateCallerIsSignable());
    return std::make_pair(nominal, nominal);
  }

  outcome::result<void> unlockBalance(State &state,
                                      const Address &address,
                                      TokenAmount amount) {
    if (amount < 0) {
      return VMExitCode::kMarketActorIllegalState;
    }
    return state.locked_table.subtract(address, amount);
  }

  outcome::result<void> slashBalance(State &state,
                                     const Address &address,
                                     TokenAmount amount) {
    VM_ASSERT(amount >= 0);
    OUTCOME_TRY(state.escrow_table.subtract(address, amount));
    OUTCOME_TRY(state.locked_table.subtract(address, amount));
    return outcome::success();
  }

  outcome::result<void> removeDeal(State &state, DealId deal_id) {
    OUTCOME_TRY(state.proposals.remove(deal_id));
    return outcome::success();
  }

  outcome::result<void> transferBalance(State &state,
                                        const Address &from,
                                        const Address &to,
                                        TokenAmount amount) {
    VM_ASSERT(amount >= 0);
    OUTCOME_TRY(state.escrow_table.subtract(from, amount));
    OUTCOME_TRY(state.locked_table.subtract(from, amount));
    OUTCOME_TRY(state.escrow_table.add(to, amount));
    return outcome::success();
  }

  outcome::result<TokenAmount> processDealInitTimedOut(
      State &state, DealId deal_id, const DealProposal &deal) {
    OUTCOME_TRY(
        unlockBalance(state, deal.client, deal.clientBalanceRequirement()));
    auto slashed{
        collateralPenaltyForDealActivationMissed(deal.provider_collateral)};
    OUTCOME_TRY(slashBalance(state, deal.provider, slashed));
    OUTCOME_TRY(unlockBalance(
        state, deal.provider, deal.providerBalanceRequirement() - slashed));
    OUTCOME_TRY(removeDeal(state, deal_id));
    return slashed;
  }

  outcome::result<std::pair<TokenAmount, ChainEpoch>> updatePendingDealState(
      State &state,
      DealId deal_id,
      const DealProposal &deal,
      const DealState &deal_state,
      ChainEpoch epoch) {
    std::pair<TokenAmount, ChainEpoch> result{0, kChainEpochUndefined};
    auto updated{deal_state.last_updated_epoch != kChainEpochUndefined};
    auto slashed{deal_state.slash_epoch != kChainEpochUndefined};
    VM_ASSERT(!updated || deal_state.last_updated_epoch <= epoch);
    if (deal.start_epoch > epoch) {
      return result;
    }
    VM_ASSERT(!slashed || deal_state.slash_epoch <= deal.end_epoch);
    OUTCOME_TRY(transferBalance(
        state,
        deal.client,
        deal.provider,
        deal.storage_price_per_epoch
            * (std::min(epoch,
                        slashed ? deal_state.slash_epoch : deal.end_epoch)
               - (updated ? std::max(deal_state.last_updated_epoch,
                                     deal.start_epoch)
                          : deal.start_epoch))));
    if (slashed) {
      VM_ASSERT(deal_state.slash_epoch <= deal.end_epoch);
      TokenAmount remaining{deal.storage_price_per_epoch
                            * (deal.end_epoch - deal_state.slash_epoch + 1)};
      OUTCOME_TRY(unlockBalance(
          state, deal.client, deal.client_collateral + remaining));
      result.first = deal.provider_collateral;
      OUTCOME_TRY(slashBalance(state, deal.provider, result.first));
      OUTCOME_TRY(removeDeal(state, deal_id));
      return result;
    }
    if (epoch >= deal.end_epoch) {
      VM_ASSERT(deal_state.sector_start_epoch != kChainEpochUndefined);
      OUTCOME_TRY(
          unlockBalance(state, deal.provider, deal.provider_collateral));
      OUTCOME_TRY(unlockBalance(state, deal.client, deal.client_collateral));
      OUTCOME_TRY(removeDeal(state, deal_id));
      return result;
    }
    result.second = std::min(deal.end_epoch, epoch + kDealUpdatesInterval);
    return result;
  }

  outcome::result<void> maybeLockBalance(State &state,
                                         const Address &address,
                                         TokenAmount amount) {
    VM_ASSERT(amount >= 0);
    OUTCOME_TRY(escrow, state.escrow_table.get(address));
    OUTCOME_TRY(locked, state.locked_table.get(address));
    if (locked + amount > escrow) {
      return VMExitCode::kMarketActorInsufficientFunds;
    }
    OUTCOME_TRY(state.locked_table.add(address, amount));
    return outcome::success();
  }

  outcome::result<void> validateDeal(Runtime &runtime,
                                     const ClientDealProposal &proposal) {
    auto &deal = proposal.proposal;
    auto duration = deal.duration();
    if (duration <= 0) {
      return VMExitCode::kMarketActorIllegalArgument;
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
      return VMExitCode::kMarketActorIllegalArgument;
    }
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kSystemActorAddress));
    State state{
        .proposals = {},
        .states = {},
        .escrow_table = {},
        .locked_table = {},
        .next_deal = 0,
        .deals_by_epoch = {},
        .last_cron = kChainEpochUndefined,
    };
    IpldPtr {
      runtime
    }
    ->load(state);
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(AddBalance) {
    OUTCOME_TRY(addresses, escrowAddress(runtime, params));
    OUTCOME_TRY(state, loadState(runtime));
    OUTCOME_TRY(state.escrow_table.addCreate(addresses.first,
                                             runtime.getValueReceived()));
    OUTCOME_TRY(state.locked_table.addCreate(addresses.first, 0));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(WithdrawBalance) {
    VM_ASSERT(params.amount >= 0);
    OUTCOME_TRY(addresses, escrowAddress(runtime, params.address));
    OUTCOME_TRY(state, loadState(runtime));
    OUTCOME_TRY(min, state.locked_table.get(addresses.first));
    OUTCOME_TRY(extracted,
                state.escrow_table.subtractWithMin(
                    addresses.first, params.amount, min));
    OUTCOME_TRY(runtime.commitState(state));
    OUTCOME_TRY(runtime.sendFunds(addresses.second, extracted));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(PublishStorageDeals) {
    OUTCOME_TRY(runtime.validateImmediateCallerIsSignable());
    VM_ASSERT(!params.deals.empty());
    auto provider_raw{params.deals[0].proposal.provider};
    OUTCOME_TRY(provider, runtime.resolveAddress(provider_raw));
    OUTCOME_TRY(addresses, requestMinerControlAddress(runtime, provider));
    OUTCOME_TRY(runtime.validateImmediateCallerIs(addresses.worker));
    for (auto &proposal : params.deals) {
      auto &deal = proposal.proposal;
      if (deal.verified) {
        OUTCOME_TRY(runtime.sendM<verified_registry::UseBytes>(
            kVerifiedRegistryAddress,
            {deal.client, uint64_t{deal.piece_size}},
            0));
      }
    }
    std::vector<DealId> deals;
    OUTCOME_TRY(state, loadState(runtime));
    for (auto &proposal : params.deals) {
      OUTCOME_TRY(validateDeal(runtime, proposal));

      auto deal{proposal.proposal};
      VM_ASSERT(deal.provider == provider || deal.provider == provider_raw);

      OUTCOME_TRY(client, runtime.resolveAddress(deal.client));
      deal.provider = provider;
      deal.client = client;

      OUTCOME_TRY(
          maybeLockBalance(state, client, deal.clientBalanceRequirement()));
      OUTCOME_TRY(
          maybeLockBalance(state, provider, deal.providerBalanceRequirement()));

      auto deal_id = state.next_deal++;
      OUTCOME_TRY(state.proposals.set(deal_id, deal));
      OUTCOME_TRY(set, state.deals_by_epoch.tryGet(deal.start_epoch));
      if (!set) {
        set = State::DealSet{IpldPtr{runtime}};
      }
      OUTCOME_TRY(set->set(deal_id, {}));
      OUTCOME_TRY(state.deals_by_epoch.set(deal.start_epoch, *set));
      deals.emplace_back(deal_id);
    }
    OUTCOME_TRY(runtime.commitState(state));
    return Result{.deals = deals};
  }

  ACTOR_METHOD_IMPL(VerifyDealsOnSectorProveCommit) {
    OUTCOME_TRY(runtime.validateImmediateCallerType(kStorageMinerCodeCid));
    auto miner = runtime.getImmediateCaller();
    OUTCOME_TRY(state, loadState(runtime));
    DealWeight weight, verified_weight;
    for (auto deal_id : params.deals) {
      OUTCOME_TRY(has_state, state.states.has(deal_id));
      VM_ASSERT(!has_state);
      OUTCOME_TRY(deal, state.proposals.get(deal_id));

      VM_ASSERT(deal.provider == miner);
      VM_ASSERT(runtime.getCurrentEpoch() <= deal.start_epoch);
      VM_ASSERT(deal.end_epoch <= params.sector_expiry);

      OUTCOME_TRY(state.states.set(deal_id,
                                   {runtime.getCurrentEpoch(),
                                    kChainEpochUndefined,
                                    kChainEpochUndefined}));
      (deal.verified ? verified_weight : weight) +=
          deal.piece_size * deal.duration();
    }
    OUTCOME_TRY(runtime.commitState(state));
    return Result{weight, verified_weight};
  }

  ACTOR_METHOD_IMPL(OnMinerSectorsTerminate) {
    OUTCOME_TRY(runtime.validateImmediateCallerType(kStorageMinerCodeCid));
    auto miner = runtime.getImmediateCaller();
    OUTCOME_TRY(state, loadState(runtime));
    for (auto deal_id : params.deals) {
      OUTCOME_TRY(deal, state.proposals.get(deal_id));
      VM_ASSERT(deal.provider == miner);
      OUTCOME_TRY(deal_state, state.states.get(deal_id));
      deal_state.slash_epoch = runtime.getCurrentEpoch();
      OUTCOME_TRY(state.states.set(deal_id, deal_state));
    }
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(ComputeDataCommitment) {
    OUTCOME_TRY(runtime.validateImmediateCallerType(kStorageMinerCodeCid));
    OUTCOME_TRY(state, loadState(runtime));
    std::vector<PieceInfo> pieces;
    for (auto deal_id : params.deals) {
      OUTCOME_TRY(deal, state.proposals.get(deal_id));
      pieces.emplace_back(
          PieceInfo{.size = deal.piece_size, .cid = deal.piece_cid});
    }
    return runtime.computeUnsealedSectorCid(params.sector_type, pieces);
  }

  ACTOR_METHOD_IMPL(CronTick) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kCronAddress));
    auto now{runtime.getCurrentEpoch()};
    OUTCOME_TRY(state, loadState(runtime));
    TokenAmount slashed_sum;
    std::map<ChainEpoch, std::vector<DealId>> next_updates;
    std::vector<DealProposal> timed_out_verified;
    for (auto epoch{state.last_cron + 1}; epoch <= now; ++epoch) {
      OUTCOME_TRY(set, state.deals_by_epoch.tryGet(epoch));
      if (set) {
        auto visitor{[&](auto deal_id, auto) -> outcome::result<void> {
          OUTCOME_TRY(deal_state, state.states.tryGet(deal_id));
          if (deal_state) {
            OUTCOME_TRY(deal, state.proposals.get(deal_id));
            if (deal_state->sector_start_epoch == kChainEpochUndefined) {
              VM_ASSERT(now >= deal.start_epoch);
              OUTCOME_TRY(slashed,
                          processDealInitTimedOut(state, deal_id, deal));
              slashed_sum += slashed;
              if (deal.verified) {
                timed_out_verified.push_back(deal);
              }
            } else {
              OUTCOME_TRY(slashed_next,
                          updatePendingDealState(
                              state, deal_id, deal, *deal_state, now));
              slashed_sum += slashed_next.first;
              if (slashed_next.second != kChainEpochUndefined) {
                VM_ASSERT(slashed_next.second > now);
                deal_state->last_updated_epoch = now;
                OUTCOME_TRY(state.states.set(deal_id, *deal_state));
                next_updates[slashed_next.second].push_back(deal_id);
              }
            }
          }
          return outcome::success();
        }};
        OUTCOME_TRY(set->visit(visitor));
        OUTCOME_TRY(state.deals_by_epoch.remove(epoch));
      }
    }
    for (auto &[next, deals] : next_updates) {
      OUTCOME_TRY(set, state.deals_by_epoch.tryGet(next));
      if (!set) {
        set = State::DealSet{IpldPtr{runtime}};
      }
      for (auto deal : deals) {
        OUTCOME_TRY(set->set(deal, {}));
      }
      OUTCOME_TRY(state.deals_by_epoch.set(next, *set));
    }
    state.last_cron = now;
    OUTCOME_TRY(runtime.commitState(state));
    for (auto &deal : timed_out_verified) {
      OUTCOME_TRY(runtime.sendM<verified_registry::RestoreBytes>(
          kVerifiedRegistryAddress,
          {deal.client, uint64_t{deal.piece_size}},
          0));
    }
    OUTCOME_TRY(runtime.sendFunds(kBurntFundsActorAddress, slashed_sum));
    return outcome::success();
  }

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<AddBalance>(),
      exportMethod<WithdrawBalance>(),
      exportMethod<PublishStorageDeals>(),
      exportMethod<VerifyDealsOnSectorProveCommit>(),
      exportMethod<OnMinerSectorsTerminate>(),
      exportMethod<ComputeDataCommitment>(),
      exportMethod<CronTick>(),
  };
}  // namespace fc::vm::actor::builtin::market
