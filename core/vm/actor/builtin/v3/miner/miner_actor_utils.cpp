/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/miner/miner_actor_utils.hpp"

#include "vm/actor/builtin/v3/account/account_actor.hpp"
#include "vm/actor/builtin/v3/market/market_actor.hpp"
#include "vm/actor/builtin/v3/reward/reward_actor.hpp"
#include "vm/actor/builtin/v3/storage_power/storage_power_actor.hpp"

namespace fc::vm::actor::builtin::v3::miner {

  outcome::result<Universal<SectorOnChainInfo>>
  MinerUtils::validateReplaceSector(MinerActorStatePtr &state,
                                    const SectorPreCommitInfo &params) const {
    CHANGE_ERROR_A(replace_sector,
                   state->sectors.sectors.get(params.replace_sector),
                   VMExitCode::kErrNotFound);

    VALIDATE_ARG(replace_sector->deals.empty());

    REQUIRE_NO_ERROR_A(replace_post_proof,
                       getRegisteredWindowPoStProof(replace_sector->seal_proof),
                       VMExitCode::kErrIllegalState);
    REQUIRE_NO_ERROR_A(new_post_proof,
                       getRegisteredWindowPoStProof(params.registered_proof),
                       VMExitCode::kErrIllegalArgument);
    VALIDATE_ARG(new_post_proof == replace_post_proof);

    VALIDATE_ARG(params.expiration >= replace_sector->expiration);

    REQUIRE_NO_ERROR(state->checkSectorHealth(params.replace_deadline,
                                              params.replace_partition,
                                              params.replace_sector),
                     VMExitCode::kErrIllegalState);

    return Universal<SectorOnChainInfo>{ActorVersion::kVersion3};
  }

  outcome::result<void> MinerUtils::canPreCommitSealProof(
      RegisteredSealProof seal_proof_type,
      NetworkVersion network_version) const {
    // Do nothing for v3
    return outcome::success();
  }

  outcome::result<EpochReward> MinerUtils::requestCurrentEpochBlockReward()
      const {
    REQUIRE_SUCCESS_A(
        reward,
        getRuntime().sendM<reward::ThisEpochReward>(kRewardAddress, {}, 0));
    return EpochReward{
        .this_epoch_reward = 0,
        .this_epoch_reward_smoothed = reward.this_epoch_reward_smoothed,
        .this_epoch_baseline_power = reward.this_epoch_baseline_power};
  }

  outcome::result<TotalPower> MinerUtils::requestCurrentTotalPower() const {
    REQUIRE_SUCCESS_A(power,
                      getRuntime().sendM<storage_power::CurrentTotalPower>(
                          kStoragePowerAddress, {}, 0));
    return TotalPower{
        .raw_byte_power = power.raw_byte_power,
        .quality_adj_power = power.quality_adj_power,
        .pledge_collateral = power.pledge_collateral,
        .quality_adj_power_smoothed = power.quality_adj_power_smoothed};
  }

  outcome::result<void> MinerUtils::notifyPledgeChanged(
      const TokenAmount &pledge_delta) const {
    if (pledge_delta != 0) {
      REQUIRE_SUCCESS(getRuntime().sendM<storage_power::UpdatePledgeTotal>(
          kStoragePowerAddress, pledge_delta, 0));
    }
    return outcome::success();
  }

  outcome::result<DealWeights> MinerUtils::requestDealWeight(
      const std::vector<DealId> &deals,
      ChainEpoch sector_start,
      ChainEpoch sector_expiry) const {
    REQUIRE_SUCCESS_A(
        sector_weights,
        getRuntime().sendM<market::VerifyDealsForActivation>(
            kStorageMarketAddress,
            {{{.sector_expiry = sector_expiry, .deal_ids = deals}}},
            0));
    return DealWeights{.deal_weight = 0,
                       .verified_deal_weight = 0,
                       .deal_space = 0,
                       .sectors = sector_weights.sectors};
  }

  outcome::result<Address> MinerUtils::getPubkeyAddressFromAccountActor(
      const Address &address) const {
    return getRuntime().sendM<account::PubkeyAddress>(address, {}, 0);
  }

  outcome::result<void> MinerUtils::callPowerEnrollCronEvent(
      ChainEpoch event_epoch, const Bytes &params) const {
    OUTCOME_TRY(getRuntime().sendM<storage_power::EnrollCronEvent>(
        kStoragePowerAddress, {event_epoch, params}, 0));
    return outcome::success();
  }

  outcome::result<void> MinerUtils::callPowerUpdateClaimedPower(
      const PowerPair &delta) const {
    OUTCOME_TRY(getRuntime().sendM<storage_power::UpdateClaimedPower>(
        kStoragePowerAddress, {delta.raw, delta.qa}, 0));
    return outcome::success();
  }

}  // namespace fc::vm::actor::builtin::v3::miner
