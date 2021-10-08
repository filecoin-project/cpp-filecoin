/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/miner/miner_actor_utils.hpp"

#include "vm/actor/builtin/v2/account/account_actor.hpp"
#include "vm/actor/builtin/v2/reward/reward_actor.hpp"
#include "vm/actor/builtin/v2/storage_power/storage_power_actor.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  using namespace types::miner;

  uint64_t MinerUtils::getAddressedPartitionsMax() const {
    return kMaxPartitionsPerDeadline;
  }

  ChainEpoch MinerUtils::nextProvingPeriodStart(ChainEpoch current_epoch,
                                                ChainEpoch offset) const {
    // Do nothing for v2
    return 0;
  }

  ChainEpoch MinerUtils::currentProvingPeriodStart(ChainEpoch current_epoch,
                                                   ChainEpoch offset) const {
    const auto current_modulus = current_epoch % kWPoStProvingPeriod;
    const ChainEpoch period_progress =
        current_modulus >= offset
            ? current_modulus - offset
            : kWPoStProvingPeriod - (offset - current_modulus);
    const ChainEpoch period_start = current_epoch - period_progress;
    return period_start;
  }

  outcome::result<uint64_t> MinerUtils::currentDeadlineIndex(
      ChainEpoch current_epoch, ChainEpoch period_start) const {
    UTILS_VM_ASSERT(current_epoch >= period_start);
    return (current_epoch - period_start) / kWPoStChallengeWindow;
  }

  outcome::result<void> MinerUtils::canPreCommitSealProof(
      RegisteredSealProof seal_proof_type,
      NetworkVersion network_version) const {
    if (network_version < NetworkVersion::kVersion7) {
      OUTCOME_TRY(getRuntime().validateArgument(
          kPreCommitSealProofTypesV0.find(seal_proof_type)
          != kPreCommitSealProofTypesV0.end()));
    } else if (network_version == NetworkVersion::kVersion7) {
      OUTCOME_TRY(getRuntime().validateArgument(
          kPreCommitSealProofTypesV7.find(seal_proof_type)
          != kPreCommitSealProofTypesV7.end()));
    } else if (network_version >= NetworkVersion::kVersion8) {
      OUTCOME_TRY(getRuntime().validateArgument(
          kPreCommitSealProofTypesV8.find(seal_proof_type)
          != kPreCommitSealProofTypesV8.end()));
    }
    return outcome::success();
  }

  outcome::result<void> MinerUtils::checkPeerInfo(
      const Buffer &peer_id,
      const std::vector<Multiaddress> &multiaddresses) const {
    OUTCOME_TRY(
        getRuntime().validateArgument(peer_id.size() <= kMaxPeerIDLength));
    size_t total_size = 0;
    for (const auto &multiaddress : multiaddresses) {
      OUTCOME_TRY(getRuntime().validateArgument(
          !multiaddress.getBytesAddress().empty()));
      total_size += multiaddress.getBytesAddress().size();
    }
    OUTCOME_TRY(
        getRuntime().validateArgument(total_size <= kMaxMultiaddressData));
    return outcome::success();
  }

  outcome::result<void> MinerUtils::checkControlAddresses(
      const std::vector<Address> &control_addresses) const {
    return getRuntime().validateArgument(control_addresses.size()
                                         <= kMaxControlAddresses);
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

  outcome::result<Address> MinerUtils::getPubkeyAddressFromAccountActor(
      const Address &address) const {
    return getRuntime().sendM<account::PubkeyAddress>(address, {}, 0);
  }

  outcome::result<void> MinerUtils::callPowerEnrollCronEvent(
      ChainEpoch event_epoch, const Buffer &params) const {
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

}  // namespace fc::vm::actor::builtin::v2::miner
