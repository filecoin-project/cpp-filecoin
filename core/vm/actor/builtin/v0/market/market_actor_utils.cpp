/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/market/market_actor_utils.hpp"

#include <boost/endian/conversion.hpp>

#include "crypto/randomness/randomness_types.hpp"
#include "primitives/cid/comm_cid.hpp"
#include "vm/actor/builtin/v0/codes.hpp"
#include "vm/actor/builtin/v0/market/policy.hpp"
#include "vm/actor/builtin/v0/shared/shared.hpp"

namespace fc::vm::actor::builtin::utils::market {
  using v0::kStorageMinerCodeCid;
  using namespace v0::market;
  using crypto::randomness::DomainSeparationTag;
  using libp2p::multi::HashType;
  using primitives::kChainEpochUndefined;
  using primitives::cid::kCommitmentBytesLen;

  outcome::result<std::tuple<Address, Address, std::vector<Address>>>
  escrowAddress(Runtime &runtime, const Address &address) {
    const auto nominal = runtime.resolveAddress(address);
    OUTCOME_TRY(runtime.validateArgument(!nominal.has_error()));

    const auto code = runtime.getActorCodeID(nominal.value());
    OUTCOME_TRY(runtime.validateArgument(!code.has_error()));

    if (code.value() == kStorageMinerCodeCid) {
      OUTCOME_TRY(miner,
                  v0::requestMinerControlAddress(runtime, nominal.value()));
      return std::make_tuple(nominal.value(),
                             miner.owner,
                             std::vector<Address>{miner.owner, miner.worker});
    }
    return std::make_tuple(nominal.value(),
                           nominal.value(),
                           std::vector<Address>{nominal.value()});
  }

  outcome::result<void> unlockBalance(const Runtime &runtime,
                                      State &state,
                                      const Address &address,
                                      const TokenAmount &amount,
                                      BalanceLockingReason lock_reason) {
    VM_ASSERT(amount >= 0);

    OUTCOME_TRY(state.locked_table.subtract(address, amount));

    switch (lock_reason) {
      case BalanceLockingReason::kClientCollateral:
        state.total_client_locked_collateral -= amount;
        break;
      case BalanceLockingReason::kClientStorageFee:
        state.total_client_storage_fee -= amount;
        break;
      case BalanceLockingReason::kProviderCollateral:
        state.total_provider_locked_collateral -= amount;
        break;
    }
    return outcome::success();
  }

  outcome::result<void> slashBalance(const Runtime &runtime,
                                     State &state,
                                     const Address &address,
                                     const TokenAmount &amount,
                                     BalanceLockingReason reason) {
    VM_ASSERT(amount >= 0);
    OUTCOME_TRY(state.escrow_table.subtract(address, amount));
    return unlockBalance(runtime, state, address, amount, reason);
  }

  outcome::result<void> transferBalance(const Runtime &runtime,
                                        State &state,
                                        const Address &from,
                                        const Address &to,
                                        const TokenAmount &amount) {
    VM_ASSERT(amount >= 0);
    CHANGE_ERROR_ABORT(state.escrow_table.subtract(from, amount),
                       VMExitCode::kErrIllegalState);
    CHANGE_ERROR_ABORT(unlockBalance(runtime,
                                     state,
                                     from,
                                     amount,
                                     BalanceLockingReason::kClientStorageFee),
                       VMExitCode::kErrIllegalState);
    CHANGE_ERROR_ABORT(state.escrow_table.add(to, amount),
                       VMExitCode::kErrIllegalState);
    return outcome::success();
  }

  outcome::result<TokenAmount> processDealInitTimedOut(
      const Runtime &runtime, State &state, const DealProposal &deal) {
    CHANGE_ERROR_ABORT(unlockBalance(runtime,
                                     state,
                                     deal.client,
                                     deal.getTotalStorageFee(),
                                     BalanceLockingReason::kClientStorageFee),
                       VMExitCode::kErrIllegalState);
    CHANGE_ERROR_ABORT(unlockBalance(runtime,
                                     state,
                                     deal.client,
                                     deal.client_collateral,
                                     BalanceLockingReason::kClientCollateral),
                       VMExitCode::kErrIllegalState);

    const auto slashed =
        collateralPenaltyForDealActivationMissed(deal.provider_collateral);
    const auto amount_remaining = deal.providerBalanceRequirement() - slashed;

    CHANGE_ERROR_ABORT(slashBalance(runtime,
                                    state,
                                    deal.provider,
                                    slashed,
                                    BalanceLockingReason::kProviderCollateral),
                       VMExitCode::kErrIllegalState);
    CHANGE_ERROR_ABORT(unlockBalance(runtime,
                                     state,
                                     deal.provider,
                                     amount_remaining,
                                     BalanceLockingReason::kProviderCollateral),
                       VMExitCode::kErrIllegalState);

    return slashed;
  }

  outcome::result<void> processDealExpired(const Runtime &runtime,
                                           State &state,
                                           const DealProposal &deal,
                                           const DealState &deal_state) {
    VM_ASSERT(deal_state.sector_start_epoch != kChainEpochUndefined);

    CHANGE_ERROR_ABORT(unlockBalance(runtime,
                                     state,
                                     deal.provider,
                                     deal.provider_collateral,
                                     BalanceLockingReason::kProviderCollateral),
                       VMExitCode::kErrIllegalState);
    CHANGE_ERROR_ABORT(unlockBalance(runtime,
                                     state,
                                     deal.client,
                                     deal.client_collateral,
                                     BalanceLockingReason::kClientCollateral),
                       VMExitCode::kErrIllegalState);

    return outcome::success();
  }

  outcome::result<void> dealProposalIsInternallyValid(
      Runtime &runtime, const ClientDealProposal &client_deal) {
    OUTCOME_TRY(buf, codec::cbor::encode(client_deal.proposal));
    OUTCOME_TRY(
        verified,
        runtime.verifySignature(
            client_deal.client_signature, client_deal.proposal.client, buf));
    OUTCOME_TRY(runtime.validateArgument(verified));

    return outcome::success();
  }

  outcome::result<TokenAmount> dealGetPaymentRemaining(const Runtime &runtime,
                                                       const DealProposal &deal,
                                                       ChainEpoch slash_epoch) {
    VM_ASSERT(slash_epoch <= deal.end_epoch);

    if (slash_epoch < deal.start_epoch) {
      slash_epoch = deal.start_epoch;
    }

    const auto duration_remaining = deal.end_epoch - slash_epoch;
    VM_ASSERT(duration_remaining >= 0);

    return duration_remaining * deal.storage_price_per_epoch;
  }

  outcome::result<std::tuple<TokenAmount, ChainEpoch, bool>>
  updatePendingDealState(const Runtime &runtime,
                         State &state,
                         DealId deal_id,
                         const DealProposal &deal,
                         const DealState &deal_state,
                         ChainEpoch epoch) {
    // std::pair<TokenAmount, ChainEpoch> result{0, kChainEpochUndefined};

    TokenAmount slashed_sum;

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
      OUTCOME_TRY(transferBalance(
          runtime, state, deal.client, deal.provider, total_payment));
    }

    if (slashed) {
      OUTCOME_TRY(
          remaining,
          dealGetPaymentRemaining(runtime, deal, deal_state.slash_epoch));

      CHANGE_ERROR_ABORT(unlockBalance(runtime,
                                       state,
                                       deal.client,
                                       remaining,
                                       BalanceLockingReason::kClientStorageFee),
                         VMExitCode::kErrIllegalState);

      CHANGE_ERROR_ABORT(unlockBalance(runtime,
                                       state,
                                       deal.client,
                                       deal.client_collateral,
                                       BalanceLockingReason::kClientCollateral),
                         VMExitCode::kErrIllegalState);

      slashed_sum = deal.provider_collateral;

      CHANGE_ERROR_ABORT(
          slashBalance(runtime,
                       state,
                       deal.provider,
                       slashed_sum,
                       BalanceLockingReason::kProviderCollateral),
          VMExitCode::kErrIllegalState);

      return std::make_tuple(slashed_sum, kChainEpochUndefined, true);
    }

    if (epoch >= deal.end_epoch) {
      OUTCOME_TRY(processDealExpired(runtime, state, deal, deal_state));
      return std::make_tuple(slashed_sum, kChainEpochUndefined, true);
    }

    const auto next_epoch = epoch + kDealUpdatesInterval;

    return std::make_tuple(slashed_sum, next_epoch, false);
  }

  outcome::result<void> maybeLockBalance(const Runtime &runtime,
                                         State &state,
                                         const Address &address,
                                         const TokenAmount &amount) {
    VM_ASSERT(amount >= 0);

    CHANGE_ERROR_A(
        locked, state.locked_table.get(address), VMExitCode::kErrIllegalState);
    CHANGE_ERROR_A(
        escrow, state.escrow_table.get(address), VMExitCode::kErrIllegalState);

    if ((locked + amount) > escrow) {
      return VMExitCode::kErrInsufficientFunds;
    }

    CHANGE_ERROR(state.locked_table.add(address, amount),
                 VMExitCode::kErrIllegalState);

    return outcome::success();
  }

  outcome::result<void> lockClientAndProviderBalances(
      const Runtime &runtime, State &state, const DealProposal &deal) {
    OUTCOME_TRY(maybeLockBalance(
        runtime, state, deal.client, deal.clientBalanceRequirement()));
    OUTCOME_TRY(maybeLockBalance(
        runtime, state, deal.provider, deal.providerBalanceRequirement()));
    state.total_client_locked_collateral += deal.client_collateral;
    state.total_client_storage_fee += deal.getTotalStorageFee();
    state.total_provider_locked_collateral += deal.provider_collateral;
    return outcome::success();
  }

  outcome::result<ChainEpoch> genRandNextEpoch(const Runtime &runtime,
                                               const DealProposal &deal) {
    OUTCOME_TRY(bytes, codec::cbor::encode(deal));
    OUTCOME_TRY(
        randomness,
        runtime.getRandomnessFromBeacon(DomainSeparationTag::MarketDealCronSeed,
                                        runtime.getCurrentEpoch() - 1,
                                        bytes));
    const uint64_t offset = boost::endian::load_big_u64(randomness.data());
    return deal.start_epoch + offset % kDealUpdatesInterval;
  }

  outcome::result<void> deleteDealProposalAndState(State &state,
                                                   DealId deal_id,
                                                   bool remove_proposal,
                                                   bool remove_state) {
    if (remove_proposal) {
      OUTCOME_TRY(state.proposals.remove(deal_id));
    }

    if (remove_state) {
      OUTCOME_TRY(state.states.remove(deal_id));
    }

    return outcome::success();
  }

  outcome::result<void> validateDealCanActivate(
      const DealProposal &deal,
      const Address &miner,
      const ChainEpoch &sector_expiration,
      const ChainEpoch &current_epoch) {
    if (deal.provider != miner) {
      return VMExitCode::kErrForbidden;
    }

    if (current_epoch > deal.start_epoch) {
      return VMExitCode::kErrIllegalArgument;
    }

    if (deal.end_epoch > sector_expiration) {
      return VMExitCode::kErrIllegalArgument;
    }

    return outcome::success();
  }

  outcome::result<void> validateDeal(Runtime &runtime,
                                     const ClientDealProposal &client_deal,
                                     const StoragePower &baseline_power,
                                     const StoragePower &network_raw_power,
                                     const StoragePower &network_qa_power) {
    CHANGE_ERROR_ABORT(dealProposalIsInternallyValid(runtime, client_deal),
                       VMExitCode::kErrIllegalArgument);

    const auto &proposal = client_deal.proposal;
    CHANGE_ERROR_ABORT(proposal.piece_size.validate(),
                       VMExitCode::kErrIllegalArgument);
    OUTCOME_TRY(runtime.validateArgument(proposal.piece_cid != CID()));

    // TODO (a.chernyshov or m.tagirov) check if proposal.piece_cid is defined
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

    OUTCOME_TRY(runtime.validateArgument(runtime.getCurrentEpoch()
                                         <= proposal.start_epoch));

    const auto duration = dealDurationBounds(proposal.piece_size);
    OUTCOME_TRY(runtime.validateArgument(duration.in(proposal.duration())));

    const auto price =
        dealPricePerEpochBounds(proposal.piece_size, proposal.duration());
    OUTCOME_TRY(
        runtime.validateArgument(price.in(proposal.storage_price_per_epoch)));

    OUTCOME_TRY(fil_circulating_supply, runtime.getTotalFilCirculationSupply());
    const auto provider_collateral =
        dealProviderCollateralBounds(proposal.piece_size,
                                     proposal.verified,
                                     network_raw_power,
                                     network_qa_power,
                                     baseline_power,
                                     fil_circulating_supply,
                                     runtime.getNetworkVersion());
    OUTCOME_TRY(runtime.validateArgument(
        provider_collateral.in(proposal.provider_collateral)));

    const auto client_collateral =
        dealClientCollateralBounds(proposal.piece_size, proposal.duration());
    OUTCOME_TRY(runtime.validateArgument(
        client_collateral.in(proposal.client_collateral)));

    return outcome::success();
  }

  outcome::result<std::tuple<DealWeight, DealWeight>>
  validateDealsForActivation(const Runtime &runtime,
                             State &state,
                             const std::vector<DealId> &deals,
                             const ChainEpoch &sector_expiry) {
    const auto miner = runtime.getImmediateCaller();

    // Lotus gas conformance
    OUTCOME_TRY(state.proposals.amt.loadRoot());

    DealWeight weight;
    DealWeight verified_weight;

    for (const auto deal_id : deals) {
      OUTCOME_TRY(deal, state.proposals.tryGet(deal_id));
      if (!deal.has_value()) {
        return VMExitCode::kErrNotFound;
      }

      OUTCOME_TRY(validateDealCanActivate(
          deal.value(), miner, sector_expiry, runtime.getCurrentEpoch()));

      if (deal->provider != miner) {
        return VMExitCode::kErrForbidden;
      }
      OUTCOME_TRY(runtime.validateArgument(runtime.getCurrentEpoch()
                                           <= deal->start_epoch));
      OUTCOME_TRY(runtime.validateArgument(deal->end_epoch <= sector_expiry));

      const auto space_time = dealWeight(deal.value());

      (deal->verified ? verified_weight : weight) += space_time;
    }
    return std::make_tuple(weight, verified_weight);
  }

}  // namespace fc::vm::actor::builtin::utils::market
