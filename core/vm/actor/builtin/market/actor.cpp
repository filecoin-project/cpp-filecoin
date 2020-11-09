/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/market/actor.hpp"

#include "common/be_decoder.hpp"
#include "crypto/hasher/hasher.hpp"
#include "crypto/randomness/randomness_types.hpp"
#include "primitives/cid/comm_cid.hpp"
#include "vm/actor/builtin/market/policy.hpp"
#include "vm/actor/builtin/reward/reward_actor.hpp"
#include "vm/actor/builtin/shared/shared.hpp"
#include "vm/actor/builtin/storage_power/storage_power_actor_export.hpp"
#include "vm/actor/builtin/verified_registry/actor.hpp"

namespace fc::vm::actor::builtin::market {
  using common::decodeBE;
  using crypto::Hasher;
  using crypto::randomness::DomainSeparationTag;
  using libp2p::multi::HashType;
  using primitives::kChainEpochUndefined;
  using primitives::cid::kCommitmentBytesLen;
  using primitives::piece::PieceInfo;

  CID DealProposal::cid() const {
    OUTCOME_EXCEPT(bytes, codec::cbor::encode(*this));
    return {CID::Version::V1,
            CID::Multicodec::DAG_CBOR,
            Hasher::blake2b_256(bytes)};
  }

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
      return std::make_pair(nominal, miner.owner);
    }
    return std::make_pair(nominal, nominal);
  }

  outcome::result<void> unlockBalance(State &state,
                                      const Address &address,
                                      TokenAmount amount) {
    if (amount < 0) {
      return VMExitCode::kErrIllegalState;
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
      State &state, const DealProposal &deal) {
    auto total_storage_fee = deal.getTotalStorageFee();
    OUTCOME_TRY(unlockBalance(state, deal.client, total_storage_fee));
    state.total_client_storage_fee -= total_storage_fee;

    auto client_collateral = deal.client_collateral;
    OUTCOME_TRY(unlockBalance(state, deal.client, client_collateral));
    state.total_client_locked_collateral -= client_collateral;

    auto slashed{
        collateralPenaltyForDealActivationMissed(deal.provider_collateral)};

    OUTCOME_TRY(slashBalance(state, deal.provider, slashed));
    state.total_provider_locked_collateral -= slashed;

    auto amount_remaining = deal.providerBalanceRequirement() - slashed;
    OUTCOME_TRY(unlockBalance(state, deal.provider, amount_remaining));
    state.total_provider_locked_collateral -= amount_remaining;

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
      return VMExitCode::kErrInsufficientFunds;
    }
    OUTCOME_TRY(state.locked_table.add(address, amount));
    return outcome::success();
  }

  outcome::result<void> validateDeal(Runtime &runtime,
                                     const ClientDealProposal &deal_proposal,
                                     const StoragePower &baseline_power,
                                     const StoragePower &network_raw_power,
                                     const StoragePower &network_qa_power) {
    const auto &proposal = deal_proposal.proposal;
    OUTCOME_TRY(encoded, codec::cbor::encode(proposal));
    OUTCOME_TRY(verified,
                runtime.verifySignature(
                    deal_proposal.client_signature, proposal.client, encoded));
    OUTCOME_TRY(runtime.validateArgument(verified));

    OUTCOME_TRY(proposal.piece_size.validate());

    // TODO (a.chernyshov) check if proposal.piece_cid is defined
    // can the CID be undef? if so, add optional<CID> to proposal and decode
    // if no, just remove the comment

    // validate CID prefix
    OUTCOME_TRY(runtime.validateArgument(
        proposal.piece_cid.version == CID::Version::V1
        && proposal.piece_cid.content_type
               == CID::Multicodec::FILECOIN_COMMITMENT_UNSEALED
        && proposal.piece_cid.content_address.getType()
               == HashType::sha2_256_trunc254_padded
        && proposal.piece_cid.content_address.getHash().size()
               == kCommitmentBytesLen));

    auto duration = proposal.duration();
    if (duration <= 0) {
      return VMExitCode::kErrIllegalArgument;
    }

    if (runtime.getCurrentEpoch() > proposal.start_epoch
        || !dealDurationBounds(proposal.piece_size).in(duration)
        || !dealPricePerEpochBounds(proposal.piece_size, duration)
                .in(proposal.storage_price_per_epoch)
        || !dealClientCollateralBounds(proposal.piece_size, duration)
                .in(proposal.client_collateral)) {
      return VMExitCode::kErrIllegalArgument;
    }

    OUTCOME_TRY(fil_circulating_supply, runtime.getTotalFilCirculationSupply());
    if (!dealProviderCollateralBounds(proposal.piece_size,
                                      proposal.verified,
                                      network_raw_power,
                                      network_qa_power,
                                      baseline_power,
                                      fil_circulating_supply,
                                      runtime.getNetworkVersion())
             .in(proposal.provider_collateral)) {
      return VMExitCode::kErrIllegalArgument;
    }
    return outcome::success();
  }

  outcome::result<std::pair<DealWeight, DealWeight>> validateDealsForActivation(
      Runtime &runtime,
      const std::vector<DealId> &deals,
      const ChainEpoch &sector_expiry) {
    auto miner = runtime.getImmediateCaller();
    OUTCOME_TRY(state, loadState(runtime));
    // load root to charge gas in order to conform lotus implementation
    OUTCOME_TRY(state.proposals.amt.loadRoot());

    DealWeight weight;
    DealWeight verified_weight;
    for (const auto deal_id : deals) {
      OUTCOME_TRY(deal, state.proposals.tryGet(deal_id));
      if (!deal.has_value()) {
        return VMExitCode::kErrNotFound;
      }

      if (deal->provider != miner) {
        return VMExitCode::kErrForbidden;
      }
      OUTCOME_TRY(runtime.validateArgument(runtime.getCurrentEpoch()
                                           <= deal->start_epoch));
      OUTCOME_TRY(runtime.validateArgument(deal->end_epoch <= sector_expiry));
      (deal->verified ? verified_weight : weight) +=
          deal->piece_size * deal->duration();
    }
    return std::make_pair(weight, verified_weight);
  }

  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kSystemActorAddress));
    State state{
        .proposals = {},
        .states = {},
        .pending_proposals = {},
        .escrow_table = {},
        .locked_table = {},
        .next_deal = 0,
        .deals_by_epoch = {},
        .last_cron = kChainEpochUndefined,
        .total_client_locked_collateral = 0,
        .total_provider_locked_collateral = 0,
        .total_client_storage_fee = 0,
    };
    IpldPtr {
      runtime
    }
    ->load(state);
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(AddBalance) {
    BigInt message_value = runtime.getValueReceived();
    OUTCOME_TRY(runtime.validateArgument(message_value > 0));
    OUTCOME_TRY(runtime.validateImmediateCallerIsSignable());
    OUTCOME_TRY(addresses, escrowAddress(runtime, params));
    OUTCOME_TRY(state, loadState(runtime));
    OUTCOME_TRY(state.escrow_table.addCreate(addresses.first, message_value));

    // load root to mutate locked_table in order to flush it in commit state to
    // conform lotus implementation
    OUTCOME_TRY(state.locked_table.hamt.loadRoot());
    // force flush to conform lotus implementation order of commitState
    OUTCOME_TRY(state.locked_table.hamt.flush());

    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(WithdrawBalance) {
    OUTCOME_TRY(runtime.validateArgument(params.amount >= 0));

    OUTCOME_TRY(addresses, escrowAddress(runtime, params.address));
    OUTCOME_TRY(
        runtime.validateImmediateCallerIs({addresses.first, addresses.second}));

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
    OUTCOME_TRY(runtime.validateArgument(!params.deals.empty()));
    auto provider_raw{params.deals[0].proposal.provider};
    OUTCOME_TRY(provider, runtime.resolveAddress(provider_raw));
    OUTCOME_TRY(code_id, runtime.getActorCodeID(provider));
    OUTCOME_TRY(runtime.validateArgument(code_id == kStorageMinerCodeCid));
    OUTCOME_TRY(addresses, requestMinerControlAddress(runtime, provider));
    OUTCOME_TRY(runtime.validateImmediateCallerIs(addresses.worker));

    // request current baseline power
    OUTCOME_TRY(epoch_reward,
                runtime.sendM<reward::ThisEpochReward>(kRewardAddress, {}, 0));
    OUTCOME_TRY(current_power,
                runtime.sendM<storage_power::CurrentTotalPower>(
                    kStoragePowerAddress, {}, 0));
    auto network_raw_power = current_power.raw_byte_power;
    auto network_qa_power = current_power.quality_adj_power;
    std::vector<DealId> deals;
    std::map<Address, Address> resolved_addresses;
    OUTCOME_TRY(state, loadState(runtime));

    // load root to mutate proposals in to charge gas to conform lotus
    // implementation
    OUTCOME_TRY(state.proposals.amt.loadRoot());
    OUTCOME_TRY(state.locked_table.hamt.loadRoot());
    OUTCOME_TRY(state.escrow_table.hamt.loadRoot());
    OUTCOME_TRY(state.pending_proposals.hamt.loadRoot());
    OUTCOME_TRY(state.deals_by_epoch.hamt.loadRoot());

    for (const auto &proposal : params.deals) {
      OUTCOME_TRY(validateDeal(runtime,
                               proposal,
                               epoch_reward.this_epoch_baseline_power,
                               network_raw_power,
                               network_qa_power));

      auto deal{proposal.proposal};
      OUTCOME_TRY(runtime.validateArgument(deal.provider == provider
                                           || deal.provider == provider_raw));

      OUTCOME_TRY(client, runtime.resolveAddress(deal.client));
      deal.provider = provider;
      resolved_addresses[deal.client] = client;
      deal.client = client;

      OUTCOME_TRY(
          maybeLockBalance(state, client, deal.clientBalanceRequirement()));
      OUTCOME_TRY(
          maybeLockBalance(state, provider, deal.providerBalanceRequirement()));
      state.total_client_locked_collateral += deal.client_collateral;
      state.total_client_storage_fee += deal.getTotalStorageFee();
      state.total_provider_locked_collateral += deal.provider_collateral;

      auto deal_id = state.next_deal++;

      OUTCOME_TRY(has, state.pending_proposals.has(deal.cid()));
      if (has) {
        return VMExitCode::kErrIllegalArgument;
      }
      OUTCOME_TRY(state.pending_proposals.set(deal.cid(), deal));

      OUTCOME_TRY(state.proposals.set(deal_id, deal));

      // We should randomize the first epoch for when the deal will be processed
      // so an attacker isn't able to schedule too many deals for the same tick.
      OUTCOME_TRY(bytes, codec::cbor::encode(deal));
      OUTCOME_TRY(randomness,
                  runtime.getRandomnessFromBeacon(
                      DomainSeparationTag::MarketDealCronSeed,
                      runtime.getCurrentEpoch() - 1,
                      bytes));
      uint64_t offset = decodeBE(randomness);
      ChainEpoch process_epoch =
          deal.start_epoch + offset % kDealUpdatesInterval;

      OUTCOME_TRY(set, state.deals_by_epoch.tryGet(process_epoch));
      if (!set) {
        set = State::DealSet{IpldPtr{runtime}};
      }
      OUTCOME_TRY(set->set(deal_id, {}));
      OUTCOME_TRY(state.deals_by_epoch.set(process_epoch, *set));
      deals.emplace_back(deal_id);

      OUTCOME_TRY(runtime.commitState(state));
    }

    for (const auto &proposal : params.deals) {
      const auto &deal = proposal.proposal;
      if (deal.verified) {
        auto resolved_client = resolved_addresses.find(deal.client);
        OUTCOME_TRY(runtime.validateArgument(resolved_client
                                             != resolved_addresses.end()));

        OUTCOME_TRY(runtime.sendM<verified_registry::UseBytes>(
            kVerifiedRegistryAddress,
            {deal.client, uint64_t{deal.piece_size}},
            0));
      }
    }

    return Result{.deals = deals};
  }

  ACTOR_METHOD_IMPL(VerifyDealsForActivation) {
    OUTCOME_TRY(runtime.validateImmediateCallerType(kStorageMinerCodeCid));
    OUTCOME_TRY(result,
                validateDealsForActivation(
                    runtime, params.deals, params.sector_expiry));
    return Result{result.first, result.second};
  }

  ACTOR_METHOD_IMPL(ActivateDeals) {
    OUTCOME_TRY(runtime.validateImmediateCallerType(kStorageMinerCodeCid));
    OUTCOME_TRY(validateDealsForActivation(
        runtime, params.deals, params.sector_expiry));

    auto miner = runtime.getImmediateCaller();
    OUTCOME_TRY(state, loadState(runtime));
    for (auto deal_id : params.deals) {
      OUTCOME_TRY(has_deal_state, state.states.has(deal_id));
      OUTCOME_TRY(runtime.validateArgument(!has_deal_state));

      OUTCOME_TRY(proposal, state.proposals.get(deal_id));
      OUTCOME_TRY(pending, state.pending_proposals.has(proposal.cid()));
      if (!pending) {
        return VMExitCode::kErrIllegalState;
      }

      DealState deal_state{
          .sector_start_epoch = runtime.getCurrentEpoch(),
          .last_updated_epoch = kChainEpochUndefined,
          .slash_epoch = kChainEpochUndefined,
      };
      OUTCOME_TRY(state.states.set(deal_id, deal_state));
    }
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(OnMinerSectorsTerminate) {
    OUTCOME_TRY(runtime.validateImmediateCallerType(kStorageMinerCodeCid));
    OUTCOME_TRY(state, loadState(runtime));

    for (auto deal_id : params.deals) {
      OUTCOME_TRY(deal, state.proposals.tryGet(deal_id));
      // Deal could have terminated and hence deleted before the sector is
      // terminated. We should simply continue instead of aborting execution
      // here if a deal is not found.
      if (!deal.has_value()) {
        continue;
      }

      OUTCOME_TRY(runtime.validateImmediateCallerIs(deal->provider));
      // do not slash expired deals
      if (deal->end_epoch <= params.epoch) {
        continue;
      }
      OUTCOME_TRY(deal_state, state.states.get(deal_id));
      // if a deal is already slashed, we don't need to do anything here
      if (deal_state.slash_epoch != kChainEpochUndefined) {
        continue;
      }
      // Mark the deal for slashing here.
      // Actual releasing of locked funds for the client and slashing of
      // provider collateral happens in CronTick.
      deal_state.slash_epoch = params.epoch;
      OUTCOME_TRY(state.states.set(deal_id, deal_state));
    }

    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(ComputeDataCommitment) {
    OUTCOME_TRY(runtime.validateImmediateCallerType(kStorageMinerCodeCid));
    OUTCOME_TRY(state, loadState(runtime));
    // load root to charge gas in order to conform lotus implementation
    OUTCOME_TRY(state.proposals.amt.loadRoot());

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
    std::map<ChainEpoch, std::vector<DealId>> updates_needed;
    std::vector<DealProposal> timed_out_verified;
    for (auto epoch{state.last_cron + 1}; epoch <= now; ++epoch) {
      OUTCOME_TRY(set, state.deals_by_epoch.tryGet(epoch));
      if (set) {
        auto visitor{[&](auto deal_id, auto) -> outcome::result<void> {
          OUTCOME_TRY(deal, state.proposals.get(deal_id));

          OUTCOME_TRY(deal_state, state.states.tryGet(deal_id));
          // deal has been published but not activated yet -> terminate it as it
          // has timed out
          if (!deal_state) {
            VM_ASSERT(now >= deal.start_epoch);
            OUTCOME_TRY(slashed, processDealInitTimedOut(state, deal));
            slashed_sum += slashed;
            if (deal.verified) {
              timed_out_verified.push_back(deal);
            }

            OUTCOME_TRY(state.proposals.remove(deal_id));
            OUTCOME_TRY(state.pending_proposals.remove(deal.cid()));
          }
          // if this is the first cron tick for the deal, it should be in the
          // pending state.
          if (deal_state->last_updated_epoch == kChainEpochUndefined) {
            OUTCOME_TRY(state.pending_proposals.remove(deal.cid()));
          }

          OUTCOME_TRY(
              slashed_next,
              updatePendingDealState(state, deal_id, deal, *deal_state, now));
          slashed_sum += slashed_next.first;
          if (slashed_next.second != kChainEpochUndefined) {
            VM_ASSERT(slashed_next.second > now);
            deal_state->last_updated_epoch = now;
            OUTCOME_TRY(state.states.set(deal_id, *deal_state));
            updates_needed[slashed_next.second].push_back(deal_id);
          }
          return outcome::success();
        }};
        OUTCOME_TRY(set->visit(visitor));
        OUTCOME_TRY(state.deals_by_epoch.remove(epoch));
      }
    }
    for (const auto &[next, deals] : updates_needed) {
      OUTCOME_TRY(set, state.deals_by_epoch.tryGet(next));
      if (!set) {
        set = State::DealSet{IpldPtr{runtime}};
      }
      for (const auto deal : deals) {
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
    if (slashed_sum != 0) {
      OUTCOME_TRY(runtime.sendFunds(kBurntFundsActorAddress, slashed_sum));
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
}  // namespace fc::vm::actor::builtin::market
