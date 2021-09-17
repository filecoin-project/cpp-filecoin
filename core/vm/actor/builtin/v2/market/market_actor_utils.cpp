/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/market/market_actor_utils.hpp"

#include <boost/endian/conversion.hpp>
#include <set>

#include "crypto/randomness/randomness_types.hpp"
#include "primitives/cid/comm_cid.hpp"
#include "vm/actor/builtin/types/market/policy.hpp"
#include "vm/actor/builtin/v2/miner/miner_actor.hpp"
#include "vm/actor/builtin/v2/reward/reward_actor.hpp"
#include "vm/actor/builtin/v2/storage_power/storage_power_actor.hpp"
#include "vm/actor/builtin/v2/verified_registry/verified_registry_actor.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::actor::builtin::v2::market {
  using crypto::randomness::DomainSeparationTag;
  using libp2p::multi::HashType;
  using primitives::cid::kCommitmentBytesLen;
  using toolchain::Toolchain;
  using namespace types::market;

  outcome::result<void> MarketUtils::checkWithdrawCaller() const {
    // Do nothing for v2
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
    OUTCOME_TRY(
        getRuntime().validateArgument(proposal.label.length() <= kDealMaxLabelSize));
    CHANGE_ERROR_ABORT(proposal.piece_size.validate(),
                       VMExitCode::kErrIllegalArgument);
    OUTCOME_TRY(getRuntime().validateArgument(proposal.piece_cid != CID()));

    // validate CID prefix
    OUTCOME_TRY(getRuntime().validateArgument(
        proposal.piece_cid.version == CID::Version::V1
        && proposal.piece_cid.content_type
               == CID::Multicodec::FILECOIN_COMMITMENT_UNSEALED
        && proposal.piece_cid.content_address.getType()
               == HashType::sha2_256_trunc254_padded
        && proposal.piece_cid.content_address.getHash().size()
               == kCommitmentBytesLen));

    OUTCOME_TRY(getRuntime().validateArgument(getRuntime().getCurrentEpoch()
                                         <= proposal.start_epoch));

    const auto duration = dealDurationBounds(proposal.piece_size);
    OUTCOME_TRY(getRuntime().validateArgument(duration.in(proposal.duration())));

    const auto price =
        dealPricePerEpochBounds(proposal.piece_size, proposal.duration());
    OUTCOME_TRY(
        getRuntime().validateArgument(price.in(proposal.storage_price_per_epoch)));

    OUTCOME_TRY(fil_circulating_supply, getRuntime().getTotalFilCirculationSupply());
    const auto provider_collateral =
        dealProviderCollateralBounds(proposal.piece_size,
                                     proposal.verified,
                                     network_raw_power,
                                     network_qa_power,
                                     baseline_power,
                                     fil_circulating_supply,
                                     getRuntime().getNetworkVersion());
    OUTCOME_TRY(getRuntime().validateArgument(
        provider_collateral.in(proposal.provider_collateral)));

    const auto client_collateral =
        dealClientCollateralBounds(proposal.piece_size, proposal.duration());
    OUTCOME_TRY(getRuntime().validateArgument(
        client_collateral.in(proposal.client_collateral)));

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

    std::set<DealId> seen_deals;

    DealWeight weight;
    DealWeight verified_weight;
    uint64_t deal_space = 0;

    for (const auto deal_id : deals) {
      // Make sure we don't double-count deals.
      const auto seen = seen_deals.insert(deal_id);
      if (!seen.second) {
        return VMExitCode::kErrIllegalArgument;
      }

      OUTCOME_TRY(deal, state->proposals.tryGet(deal_id));
      if (!deal.has_value()) {
        return VMExitCode::kErrNotFound;
      }

      OUTCOME_TRY(validateDealCanActivate(
          deal.value(), miner, sector_expiry, getRuntime().getCurrentEpoch()));

      if (deal->provider != miner) {
        return VMExitCode::kErrForbidden;
      }
      OUTCOME_TRY(getRuntime().validateArgument(getRuntime().getCurrentEpoch()
                                           <= deal->start_epoch));
      OUTCOME_TRY(getRuntime().validateArgument(deal->end_epoch <= sector_expiry));

      deal_space += deal->piece_size;
      const auto space_time = dealWeight(deal.value());

      (deal->verified ? verified_weight : weight) += space_time;
    }
    return std::make_tuple(weight, verified_weight, deal_space);
  }

  outcome::result<StoragePower> MarketUtils::getBaselinePowerFromRewardActor()
      const {
    OUTCOME_TRY(epoch_reward,
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
      const DealProposal &deal) const {
    OUTCOME_TRY(getRuntime().sendM<verified_registry::UseBytes>(
        kVerifiedRegistryAddress, {deal.client, uint64_t{deal.piece_size}}, 0));
    return outcome::success();
  }

  outcome::result<void> MarketUtils::callVerifRegRestoreBytes(
      const DealProposal &deal) const {
    OUTCOME_TRY(getRuntime().sendM<verified_registry::RestoreBytes>(
        kVerifiedRegistryAddress, {deal.client, uint64_t{deal.piece_size}}, 0));
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

}  // namespace fc::vm::actor::builtin::v2::market
