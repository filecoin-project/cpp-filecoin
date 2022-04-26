/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/market/market_actor_utils.hpp"

#include <boost/endian/conversion.hpp>

#include "crypto/randomness/randomness_types.hpp"
#include "vm/actor/builtin/types/market/policy.hpp"
#include "vm/actor/builtin/v0/miner/miner_actor.hpp"
#include "vm/actor/builtin/v0/reward/reward_actor.hpp"
#include "vm/actor/builtin/v0/storage_power/storage_power_actor.hpp"
#include "vm/actor/builtin/v0/verified_registry/verified_registry_actor.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::actor::builtin::v0::market {
  using crypto::randomness::DomainSeparationTag;
  using toolchain::Toolchain;
  using namespace types::market;

  outcome::result<void> MarketUtils::checkWithdrawCaller() const {
    return getRuntime().validateImmediateCallerIsSignable();
  }

  outcome::result<void> MarketUtils::checkCallers(
      const Address &provider) const {
    OUTCOME_TRY(addresses, requestMinerControlAddress(provider));
    if (addresses.worker != getRuntime().getImmediateCaller()) {
      ABORT(VMExitCode::kErrForbidden);
    }
    return outcome::success();
  }

  outcome::result<std::tuple<Address, Address, std::vector<Address>>>
  MarketUtils::escrowAddress(const Address &address) const {
    const auto nominal = getRuntime().resolveAddress(address);
    VALIDATE_ARG(!nominal.has_error());

    const auto code = getRuntime().getActorCodeID(nominal.value());
    VALIDATE_ARG(!code.has_error());

    const auto address_matcher =
        Toolchain::createAddressMatcher(getRuntime().getActorVersion());
    if (code.value() == address_matcher->getStorageMinerCodeId()) {
      OUTCOME_TRY(miner, requestMinerControlAddress(nominal.value()));
      return std::make_tuple(nominal.value(),
                             miner.owner,
                             std::vector<Address>{miner.owner, miner.worker});
    }
    return std::make_tuple(nominal.value(),
                           nominal.value(),
                           std::vector<Address>{nominal.value()});
  }

  outcome::result<void> MarketUtils::dealProposalIsInternallyValid(
      const ClientDealProposal &client_deal) const {
    OUTCOME_TRY(buf, codec::cbor::encode(client_deal.proposal));
    OUTCOME_TRY(
        verified,
        getRuntime().verifySignature(
            client_deal.client_signature, client_deal.proposal->client, buf));
    VALIDATE_ARG(verified);

    return outcome::success();
  }

  outcome::result<TokenAmount> MarketUtils::dealGetPaymentRemaining(
      const Universal<DealProposal> &deal, ChainEpoch slash_epoch) const {
    OUTCOME_TRY(check(slash_epoch <= deal->end_epoch));

    if (slash_epoch < deal->start_epoch) {
      slash_epoch = deal->start_epoch;
    }

    const auto duration_remaining = deal->end_epoch - slash_epoch;
    OUTCOME_TRY(check(duration_remaining >= 0));

    return duration_remaining * deal->storage_price_per_epoch;
  }

  outcome::result<ChainEpoch> MarketUtils::genRandNextEpoch(
      const Universal<DealProposal> &deal) const {
    OUTCOME_TRY(bytes, codec::cbor::encode(deal));
    OUTCOME_TRY(randomness,
                getRuntime().getRandomnessFromBeacon(
                    DomainSeparationTag::MarketDealCronSeed,
                    getRuntime().getCurrentEpoch() - 1,
                    bytes));
    const uint64_t offset = boost::endian::load_big_u64(randomness.data());
    return deal->start_epoch + offset % kDealUpdatesInterval;
  }

  outcome::result<void> MarketUtils::deleteDealProposalAndState(
      MarketActorStatePtr &state,
      DealId deal_id,
      bool remove_proposal,
      bool remove_state) const {
    if (remove_proposal) {
      OUTCOME_TRY(state->proposals.remove(deal_id));
    }

    if (remove_state) {
      OUTCOME_TRY(state->states.remove(deal_id));
    }

    return outcome::success();
  }

  outcome::result<void> MarketUtils::validateDealCanActivate(
      const Universal<DealProposal> &deal,
      const Address &miner,
      const ChainEpoch &sector_expiration,
      const ChainEpoch &current_epoch) const {
    if (deal->provider != miner) {
      return VMExitCode::kErrForbidden;
    }

    if (current_epoch > deal->start_epoch) {
      return VMExitCode::kErrIllegalArgument;
    }

    if (deal->end_epoch > sector_expiration) {
      return VMExitCode::kErrIllegalArgument;
    }

    return outcome::success();
  }

  outcome::result<void> MarketUtils::validateDeal(
      const ClientDealProposal &client_deal,
      const StoragePower &baseline_power,
      const StoragePower &network_raw_power,
      const StoragePower &network_qa_power) const {
    CHANGE_ERROR_ABORT(dealProposalIsInternallyValid(client_deal),
                       VMExitCode::kErrIllegalArgument);

    const auto &proposal = client_deal.proposal;
    CHANGE_ERROR_ABORT(proposal->piece_size.validate(),
                       VMExitCode::kErrIllegalArgument);
    VALIDATE_ARG(proposal->piece_cid != CID());

    // validate CID prefix
    VALIDATE_ARG(proposal->piece_cid.getPrefix() == kPieceCIDPrefix);

    VALIDATE_ARG(getRuntime().getCurrentEpoch() <= proposal->start_epoch);

    const auto duration = dealDurationBounds(proposal->piece_size);
    VALIDATE_ARG(duration.in(proposal->duration()));

    const auto price =
        dealPricePerEpochBounds(proposal->piece_size, proposal->duration());
    VALIDATE_ARG(price.in(proposal->storage_price_per_epoch));

    OUTCOME_TRY(fil_circulating_supply,
                getRuntime().getTotalFilCirculationSupply());
    const auto provider_collateral =
        dealProviderCollateralBounds(proposal->piece_size,
                                     proposal->verified,
                                     network_raw_power,
                                     network_qa_power,
                                     baseline_power,
                                     fil_circulating_supply,
                                     getRuntime().getNetworkVersion());
    VALIDATE_ARG(provider_collateral.in(proposal->provider_collateral));

    const auto client_collateral =
        dealClientCollateralBounds(proposal->piece_size, proposal->duration());
    VALIDATE_ARG(client_collateral.in(proposal->client_collateral));

    return outcome::success();
  }

  outcome::result<std::tuple<DealWeight, DealWeight, uint64_t>>
  MarketUtils::validateDealsForActivation(
      MarketActorStatePtr &state,
      const std::vector<DealId> &deals,
      const ChainEpoch &sector_expiry) const {
    const auto miner = getRuntime().getImmediateCaller();

    // Lotus gas conformance
    OUTCOME_TRY(state->proposals.amt.loadRoot());

    DealWeight weight;
    DealWeight verified_weight;

    for (const auto deal_id : deals) {
      OUTCOME_TRY(maybe_deal, state->proposals.tryGet(deal_id));
      if (!maybe_deal.has_value()) {
        return VMExitCode::kErrNotFound;
      }
      const auto &deal{maybe_deal.value()};

      OUTCOME_TRY(validateDealCanActivate(
          deal, miner, sector_expiry, getRuntime().getCurrentEpoch()));

      if (deal->provider != miner) {
        return VMExitCode::kErrForbidden;
      }
      VALIDATE_ARG(getRuntime().getCurrentEpoch() <= deal->start_epoch);
      VALIDATE_ARG(deal->end_epoch <= sector_expiry);

      const auto space_time = dealWeight(deal);

      (deal->verified ? verified_weight : weight) += space_time;
    }
    return std::make_tuple(weight, verified_weight, 0);
  }

  outcome::result<std::tuple<DealWeight, DealWeight, uint64_t>>
  MarketUtils::validateAndComputeDealWeight(
      DealArray &proposals,
      const std::vector<DealId> &deals,
      const ChainEpoch &sector_expiry) const {
    // Do nothing for v0
    return std::make_tuple(0, 0, 0);
  }

  outcome::result<StoragePower> MarketUtils::getBaselinePowerFromRewardActor()
      const {
    OUTCOME_TRY(
        epoch_reward,
        getRuntime().sendM<reward::ThisEpochReward>(kRewardAddress, {}, 0));
    return epoch_reward.this_epoch_baseline_power;
  }

  outcome::result<std::tuple<StoragePower, StoragePower>>
  MarketUtils::getRawAndQaPowerFromPowerActor() const {
    OUTCOME_TRY(current_power,
                getRuntime().sendM<storage_power::CurrentTotalPower>(
                    kStoragePowerAddress, {}, 0));
    return std::make_tuple(current_power.raw_byte_power,
                           current_power.quality_adj_power);
  }

  outcome::result<void> MarketUtils::callVerifRegUseBytes(
      const Universal<DealProposal> &deal) const {
    OUTCOME_TRY(getRuntime().sendM<verified_registry::UseBytes>(
        kVerifiedRegistryAddress,
        {deal->client, uint64_t{deal->piece_size}},
        0));
    return outcome::success();
  }

  outcome::result<void> MarketUtils::callVerifRegRestoreBytes(
      const Universal<DealProposal> &deal) const {
    OUTCOME_TRY(getRuntime().sendM<verified_registry::RestoreBytes>(
        kVerifiedRegistryAddress,
        {deal->client, uint64_t{deal->piece_size}},
        0));
    return outcome::success();
  }

  outcome::result<Controls> MarketUtils::requestMinerControlAddress(
      const Address &miner) const {
    OUTCOME_TRY(addresses,
                getRuntime().sendM<miner::ControlAddresses>(miner, {}, 0));
    return Controls{.owner = addresses.owner,
                    .worker = addresses.worker,
                    .control = addresses.control};
  }

}  // namespace fc::vm::actor::builtin::v0::market
