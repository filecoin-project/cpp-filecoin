/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/market/market_actor.hpp"

#include "primitives/cid/comm_cid.hpp"
#include "vm/actor/builtin/states/market/market_actor_state.hpp"
#include "vm/actor/builtin/types/market/policy.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::actor::builtin::v0::market {
  using libp2p::multi::HashType;
  using primitives::kChainEpochUndefined;
  using primitives::cid::kCommitmentBytesLen;
  using primitives::piece::PieceInfo;
  using states::DealSet;
  using states::MarketActorStatePtr;
  using toolchain::Toolchain;
  using types::Universal;
  using namespace types::market;

  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kSystemActorAddress));
    const auto actor_version = runtime.getActorVersion();
    MarketActorStatePtr state{actor_version};
    state->pending_proposals = Universal<PendingProposals>{actor_version};
    cbor_blake::cbLoadT(runtime.getIpfsDatastore(), state);
    state->last_cron = kChainEpochUndefined;

    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(AddBalance) {
    const BigInt message_value = runtime.getValueReceived();
    OUTCOME_TRY(runtime.validateArgument(message_value > 0));
    OUTCOME_TRY(runtime.validateImmediateCallerIsSignable());

    const auto utils = Toolchain::createMarketUtils(runtime);

    OUTCOME_TRY(addresses, utils->escrowAddress(params));

    Address nominal;
    std::tie(nominal, std::ignore, std::ignore) = addresses;

    REQUIRE_NO_ERROR_A(state,
                       runtime.getActorState<MarketActorStatePtr>(),
                       VMExitCode::kErrIllegalState);
    REQUIRE_NO_ERROR(state->escrow_table.addCreate(nominal, message_value),
                     VMExitCode::kErrIllegalState);

    // Lotus gas conformance
    REQUIRE_NO_ERROR(state->locked_table.hamt.loadRoot(),
                     VMExitCode::kErrIllegalState);
    // Lotus gas conformance
    REQUIRE_NO_ERROR(state->locked_table.hamt.flush(),
                     VMExitCode::kErrIllegalState);

    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(WithdrawBalance) {
    OUTCOME_TRY(runtime.validateArgument(params.amount >= 0));
    const auto utils = Toolchain::createMarketUtils(runtime);
    OUTCOME_TRY(utils->checkWithdrawCaller());
    OUTCOME_TRY(addresses, utils->escrowAddress(params.address));
    const auto &[nominal, recipient, approved_callers] = addresses;
    OUTCOME_TRY(runtime.validateImmediateCallerIs(approved_callers));

    REQUIRE_NO_ERROR_A(state,
                       runtime.getActorState<MarketActorStatePtr>(),
                       VMExitCode::kErrIllegalState);
    REQUIRE_NO_ERROR_A(
        min, state->locked_table.get(nominal), VMExitCode::kErrIllegalState);
    REQUIRE_NO_ERROR_A(
        extracted,
        state->escrow_table.subtractWithMin(nominal, params.amount, min),
        VMExitCode::kErrIllegalState);
    OUTCOME_TRY(runtime.commitState(state));
    REQUIRE_SUCCESS(runtime.sendFunds(recipient, extracted));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(PublishStorageDeals) {
    OUTCOME_TRY(runtime.validateImmediateCallerIsSignable());
    OUTCOME_TRY(runtime.validateArgument(!params.deals.empty()));

    const auto provider_raw{params.deals[0].proposal.provider};
    CHANGE_ERROR_ABORT_A(provider,
                         runtime.resolveAddress(provider_raw),
                         VMExitCode::kErrNotFound);

    const auto code_id = runtime.getActorCodeID(provider);
    OUTCOME_TRY(runtime.validateArgument(!code_id.has_error()));

    const auto address_matcher =
        Toolchain::createAddressMatcher(runtime.getActorVersion());
    OUTCOME_TRY(runtime.validateArgument(
        code_id.value() == address_matcher->getStorageMinerCodeId()));

    const auto utils = Toolchain::createMarketUtils(runtime);

    OUTCOME_TRY(addresses, utils->requestMinerControlAddress(provider));
    if (addresses.worker != runtime.getImmediateCaller()) {
      ABORT(VMExitCode::kErrForbidden);
    }

    // request current baseline power
    OUTCOME_TRY(baseline_power, utils->getBaselinePowerFromRewardActor());
    OUTCOME_TRY(current_power, utils->getRawAndQaPowerFromPowerActor());
    const auto &[network_raw_power, network_qa_power] = current_power;

    std::vector<DealId> deals;
    std::map<Address, Address> resolved_addresses;
    REQUIRE_NO_ERROR_A(state,
                       runtime.getActorState<MarketActorStatePtr>(),
                       VMExitCode::kErrIllegalState);

    // Lotus gas conformance
    REQUIRE_NO_ERROR(state->proposals.amt.loadRoot(),
                     VMExitCode::kErrIllegalState);
    REQUIRE_NO_ERROR(state->locked_table.hamt.loadRoot(),
                     VMExitCode::kErrIllegalState);
    REQUIRE_NO_ERROR(state->escrow_table.hamt.loadRoot(),
                     VMExitCode::kErrIllegalState);
    REQUIRE_NO_ERROR(state->pending_proposals->loadRoot(),
                     VMExitCode::kErrIllegalState);
    REQUIRE_NO_ERROR(state->deals_by_epoch.hamt.loadRoot(),
                     VMExitCode::kErrIllegalState);

    for (const auto &client_deals : params.deals) {
      OUTCOME_TRY(utils->validateDeal(
          client_deals, baseline_power, network_raw_power, network_qa_power));

      auto deal{client_deals.proposal};
      OUTCOME_TRY(runtime.validateArgument(deal.provider == provider
                                           || deal.provider == provider_raw));

      CHANGE_ERROR_ABORT_A(client,
                           runtime.resolveAddress(deal.client),
                           VMExitCode::kErrNotFound);
      deal.provider = provider;
      resolved_addresses[deal.client] = client;
      deal.client = client;

      REQUIRE_NO_ERROR(state->lockClientAndProviderBalances(runtime, deal),
                       VMExitCode::kErrIllegalState);

      const auto deal_id = state->next_deal++;

      REQUIRE_NO_ERROR_A(has,
                         state->pending_proposals->has(deal.cid()),
                         VMExitCode::kErrIllegalState);
      OUTCOME_TRY(runtime.validateArgument(!has));

      REQUIRE_NO_ERROR(state->pending_proposals->set(deal.cid(), deal),
                       VMExitCode::kErrIllegalState);

      REQUIRE_NO_ERROR(state->proposals.set(deal_id, deal),
                       VMExitCode::kErrIllegalState);

      // We should randomize the first epoch for when the deal will be processed
      // so an attacker isn't able to schedule too many deals for the same tick.
      REQUIRE_NO_ERROR_A(process_epoch,
                         utils->genRandNextEpoch(deal),
                         VMExitCode::kErrIllegalState);

      OUTCOME_TRY(set, state->deals_by_epoch.tryGet(process_epoch));
      if (!set) {
        set = DealSet{IpldPtr{runtime}};
      }
      OUTCOME_TRY(set->set(deal_id));
      REQUIRE_NO_ERROR(state->deals_by_epoch.set(process_epoch, *set),
                       VMExitCode::kErrIllegalState);
      deals.emplace_back(deal_id);
    }

    OUTCOME_TRY(runtime.commitState(state));

    for (const auto &client_deal : params.deals) {
      const auto &deal = client_deal.proposal;
      if (deal.verified) {
        const auto resolved_client = resolved_addresses.find(deal.client);
        OUTCOME_TRY(runtime.validateArgument(resolved_client
                                             != resolved_addresses.end()));

        REQUIRE_SUCCESS(utils->callVerifRegUseBytes(deal));
      }
    }

    return Result{.deals = deals};
  }

  outcome::result<std::tuple<DealWeight, DealWeight, uint64_t>>
  VerifyDealsForActivation::verifyDealsForActivation(
      Runtime &runtime, const VerifyDealsForActivation::Params &params) {
    const auto address_matcher =
        Toolchain::createAddressMatcher(runtime.getActorVersion());
    OUTCOME_TRY(runtime.validateImmediateCallerType(
        address_matcher->getStorageMinerCodeId()));
    REQUIRE_NO_ERROR_A(state,
                       runtime.getActorState<MarketActorStatePtr>(),
                       VMExitCode::kErrIllegalState);
    const auto utils = Toolchain::createMarketUtils(runtime);
    REQUIRE_NO_ERROR_A(result,
                       utils->validateDealsForActivation(
                           state, params.deals, params.sector_expiry),
                       VMExitCode::kErrIllegalState);
    return std::move(result);
  }

  ACTOR_METHOD_IMPL(VerifyDealsForActivation) {
    OUTCOME_TRY(result, verifyDealsForActivation(runtime, params));
    const auto &[deal_weight, verified_weight, deal_space] = result;

    return Result{deal_weight, verified_weight};
  }

  ACTOR_METHOD_IMPL(ActivateDeals) {
    const auto address_matcher =
        Toolchain::createAddressMatcher(runtime.getActorVersion());
    OUTCOME_TRY(runtime.validateImmediateCallerType(
        address_matcher->getStorageMinerCodeId()));
    REQUIRE_NO_ERROR_A(state,
                       runtime.getActorState<MarketActorStatePtr>(),
                       VMExitCode::kErrIllegalState);
    const auto utils = Toolchain::createMarketUtils(runtime);
    REQUIRE_NO_ERROR(utils->validateDealsForActivation(
                         state, params.deals, params.sector_expiry),
                     VMExitCode::kErrIllegalState);

    const auto miner = runtime.getImmediateCaller();
    for (const auto deal_id : params.deals) {
      REQUIRE_NO_ERROR_A(has_deal_state,
                         state->states.has(deal_id),
                         VMExitCode::kErrIllegalState);
      OUTCOME_TRY(runtime.validateArgument(!has_deal_state));

      REQUIRE_NO_ERROR_A(proposal,
                         state->proposals.get(deal_id),
                         VMExitCode::kErrIllegalState);

      const auto pending = state->pending_proposals->has(proposal.cid());
      if (!pending.value()) {
        ABORT(VMExitCode::kErrIllegalState);
      }

      DealState deal_state{
          .sector_start_epoch = runtime.getCurrentEpoch(),
          .last_updated_epoch = kChainEpochUndefined,
          .slash_epoch = kChainEpochUndefined,
      };
      REQUIRE_NO_ERROR(state->states.set(deal_id, deal_state),
                       VMExitCode::kErrIllegalState);
    }
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(OnMinerSectorsTerminate) {
    const auto address_matcher =
        Toolchain::createAddressMatcher(runtime.getActorVersion());
    OUTCOME_TRY(runtime.validateImmediateCallerType(
        address_matcher->getStorageMinerCodeId()));
    REQUIRE_NO_ERROR_A(state,
                       runtime.getActorState<MarketActorStatePtr>(),
                       VMExitCode::kErrIllegalState);

    for (const auto deal_id : params.deals) {
      REQUIRE_NO_ERROR_A(maybe_deal,
                         state->proposals.tryGet(deal_id),
                         VMExitCode::kErrIllegalState);

      // Deal could have terminated and hence deleted before the sector is
      // terminated. We should simply continue instead of aborting execution
      // here if a deal is not found.
      if (!maybe_deal.has_value()) {
        continue;
      }

      const auto &deal = maybe_deal.value();

      VM_ASSERT(deal.provider == runtime.getImmediateCaller());

      // do not slash expired deals
      if (deal.end_epoch <= params.epoch) {
        continue;
      }

      REQUIRE_NO_ERROR_A(maybe_deal_state,
                         state->states.tryGet(deal_id),
                         VMExitCode::kErrIllegalState);
      OUTCOME_TRY(runtime.validateArgument(maybe_deal_state.has_value()));
      auto &deal_state = maybe_deal_state.value();

      // if a deal is already slashed, we don't need to do anything here
      if (deal_state.slash_epoch != kChainEpochUndefined) {
        continue;
      }
      // Mark the deal for slashing here.
      // Actual releasing of locked funds for the client and slashing of
      // provider collateral happens in CronTick.
      deal_state.slash_epoch = params.epoch;
      REQUIRE_NO_ERROR(state->states.set(deal_id, deal_state),
                       VMExitCode::kErrIllegalState);
    }

    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(ComputeDataCommitment) {
    const auto address_matcher =
        Toolchain::createAddressMatcher(runtime.getActorVersion());
    OUTCOME_TRY(runtime.validateImmediateCallerType(
        address_matcher->getStorageMinerCodeId()));
    REQUIRE_NO_ERROR_A(state,
                       runtime.getActorState<MarketActorStatePtr>(),
                       VMExitCode::kErrIllegalState);

    // Lotus gas conformance
    REQUIRE_NO_ERROR(state->proposals.amt.loadRoot(),
                     VMExitCode::kErrIllegalState);

    std::vector<PieceInfo> pieces;
    for (const auto deal_id : params.deals) {
      REQUIRE_NO_ERROR_A(
          deal, state->proposals.get(deal_id), VMExitCode::kErrIllegalState);
      pieces.emplace_back(
          PieceInfo{.size = deal.piece_size, .cid = deal.piece_cid});
    }
    const auto result =
        runtime.computeUnsealedSectorCid(params.sector_type, pieces);
    OUTCOME_TRY(runtime.validateArgument(!result.has_error()));
    return result;
  }

  ACTOR_METHOD_IMPL(CronTick) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kCronAddress));
    const auto now{runtime.getCurrentEpoch()};
    REQUIRE_NO_ERROR_A(state,
                       runtime.getActorState<MarketActorStatePtr>(),
                       VMExitCode::kErrIllegalState);
    TokenAmount slashed_sum;
    std::map<ChainEpoch, std::vector<DealId>> updates_needed;
    std::vector<DealProposal> timed_out_verified;

    // Lotus gas conformance
    REQUIRE_NO_ERROR(state->states.amt.loadRoot(),
                     VMExitCode::kErrIllegalState);
    REQUIRE_NO_ERROR(state->locked_table.hamt.loadRoot(),
                     VMExitCode::kErrIllegalState);
    REQUIRE_NO_ERROR(state->escrow_table.hamt.loadRoot(),
                     VMExitCode::kErrIllegalState);
    REQUIRE_NO_ERROR(state->deals_by_epoch.hamt.loadRoot(),
                     VMExitCode::kErrIllegalState);
    REQUIRE_NO_ERROR(state->proposals.amt.loadRoot(),
                     VMExitCode::kErrIllegalState);
    REQUIRE_NO_ERROR(state->pending_proposals->loadRoot(),
                     VMExitCode::kErrIllegalState);

    const auto utils = Toolchain::createMarketUtils(runtime);

    for (auto epoch{state->last_cron + 1}; epoch <= now; ++epoch) {
      OUTCOME_TRY(set, state->deals_by_epoch.tryGet(epoch));
      if (set.has_value()) {
        auto visitor{[&](auto deal_id, auto) -> outcome::result<void> {
          REQUIRE_NO_ERROR_A(deal,
                             state->proposals.get(deal_id),
                             VMExitCode::kErrIllegalState);

          REQUIRE_NO_ERROR_A(maybe_deal_state,
                             state->states.tryGet(deal_id),
                             VMExitCode::kErrIllegalState);
          // deal has been published but not activated yet -> terminate it as it
          // has timed out
          if (!maybe_deal_state.has_value()) {
            VM_ASSERT(now >= deal.start_epoch);
            OUTCOME_TRY(slashed, state->processDealInitTimedOut(runtime, deal));
            slashed_sum += slashed;
            if (deal.verified) {
              timed_out_verified.push_back(deal);
            }

            REQUIRE_NO_ERROR(
                utils->deleteDealProposalAndState(state, deal_id, true, false),
                VMExitCode::kErrIllegalState);
          }

          auto &deal_state = maybe_deal_state.value();

          // if this is the first cron tick for the deal, it should be in the
          // pending state->
          if (deal_state.last_updated_epoch == kChainEpochUndefined) {
            REQUIRE_NO_ERROR(state->pending_proposals->remove(deal.cid()),
                             VMExitCode::kErrIllegalState);
          }

          OUTCOME_TRY(slashed_next,
                      state->updatePendingDealState(
                          runtime, deal_id, deal, deal_state, now));
          const auto &[slash_amount, next_epoch, remove_deal] = slashed_next;

          VM_ASSERT(slash_amount >= 0);
          if (remove_deal) {
            VM_ASSERT(next_epoch == kChainEpochUndefined);
            slashed_sum += slash_amount;
            REQUIRE_NO_ERROR(
                utils->deleteDealProposalAndState(state, deal_id, true, true),
                VMExitCode::kErrIllegalState);
          } else {
            VM_ASSERT((next_epoch > now) && (slash_amount == 0));
            deal_state.last_updated_epoch = now;
            REQUIRE_NO_ERROR(state->states.set(deal_id, deal_state),
                             VMExitCode::kErrIllegalState);
            updates_needed[next_epoch].push_back(deal_id);
          }
          return outcome::success();
        }};
        REQUIRE_NO_ERROR(set->visit(visitor), VMExitCode::kErrIllegalState);
        OUTCOME_TRY(state->deals_by_epoch.remove(epoch));
      }
    }

    for (const auto &[next, deals] : updates_needed) {
      OUTCOME_TRY(set, state->deals_by_epoch.tryGet(next));
      if (!set) {
        set = DealSet{IpldPtr{runtime}};
      }
      for (const auto deal : deals) {
        OUTCOME_TRY(set->set(deal));
      }
      OUTCOME_TRY(state->deals_by_epoch.set(next, *set));
    }

    state->last_cron = now;
    OUTCOME_TRY(runtime.commitState(state));

    for (const auto &deal : timed_out_verified) {
      OUTCOME_TRY(utils->callVerifRegRestoreBytes(deal));
    }
    if (slashed_sum != 0) {
      REQUIRE_SUCCESS(runtime.sendFunds(kBurntFundsActorAddress, slashed_sum));
    }
    return outcome::success();
  }

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<AddBalance>(),
      exportMethod<WithdrawBalance>(),
      exportMethod<PublishStorageDeals>(),
      exportMethod<VerifyDealsForActivation>(),
      exportMethod<ActivateDeals>(),
      exportMethod<OnMinerSectorsTerminate>(),
      exportMethod<ComputeDataCommitment>(),
      exportMethod<CronTick>(),
  };
}  // namespace fc::vm::actor::builtin::v0::market
