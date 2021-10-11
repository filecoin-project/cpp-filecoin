/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/miner/miner_actor_utils.hpp"

#include <boost/endian/conversion.hpp>
#include "vm/actor/builtin/types/miner/proof_policy.hpp"
#include "vm/actor/builtin/v0/account/account_actor.hpp"
#include "vm/actor/builtin/v0/market/market_actor.hpp"
#include "vm/actor/builtin/v0/reward/reward_actor.hpp"
#include "vm/actor/builtin/v0/storage_power/storage_power_actor.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::actor::builtin::v0::miner {
  using common::smoothing::FilterEstimate;
  using crypto::randomness::DomainSeparationTag;
  using primitives::sector::SectorInfo;
  using primitives::sector::WindowPoStVerifyInfo;
  using toolchain::Toolchain;
  using types::Universal;
  using namespace types::miner;

  uint64_t MinerUtils::getAddressedPartitionsMax() const {
    return 200;
  }

  outcome::result<Address> MinerUtils::resolveControlAddress(
      const Address &address) const {
    const auto resolved = getRuntime().resolveAddress(address);
    OUTCOME_TRY(getRuntime().validateArgument(!resolved.has_error()));
    UTILS_VM_ASSERT(resolved.value().isId());
    const auto resolved_code = getRuntime().getActorCodeID(resolved.value());
    OUTCOME_TRY(getRuntime().validateArgument(!resolved_code.has_error()));

    const auto address_matcher =
        Toolchain::createAddressMatcher(getRuntime().getActorVersion());
    OUTCOME_TRY(getRuntime().validateArgument(
        address_matcher->isSignableActor(resolved_code.value())));
    return std::move(resolved.value());
  }

  outcome::result<Address> MinerUtils::resolveWorkerAddress(
      const Address &address) const {
    const auto resolved = getRuntime().resolveAddress(address);
    OUTCOME_TRY(getRuntime().validateArgument(!resolved.has_error()));
    UTILS_VM_ASSERT(resolved.value().isId());
    const auto resolved_code = getRuntime().getActorCodeID(resolved.value());
    OUTCOME_TRY(getRuntime().validateArgument(!resolved_code.has_error()));
    const auto address_matcher =
        Toolchain::createAddressMatcher(getRuntime().getActorVersion());
    OUTCOME_TRY(getRuntime().validateArgument(
        resolved_code.value() == address_matcher->getAccountCodeId()));

    if (!address.isBls()) {
      OUTCOME_TRY(pubkey_addres,
                  getPubkeyAddressFromAccountActor(resolved.value()));
      OUTCOME_TRY(getRuntime().validateArgument(pubkey_addres.isBls()));
    }

    return std::move(resolved.value());
  }

  outcome::result<void> MinerUtils::enrollCronEvent(
      ChainEpoch event_epoch, const CronEventPayload &payload) const {
    const auto encoded_params = codec::cbor::encode(payload);
    OUTCOME_TRY(getRuntime().validateArgument(!encoded_params.has_error()));
    REQUIRE_SUCCESS(
        callPowerEnrollCronEvent(event_epoch, encoded_params.value()));
    return outcome::success();
  }

  outcome::result<void> MinerUtils::requestUpdatePower(
      const PowerPair &delta) const {
    if (delta.isZero()) {
      return outcome::success();
    }
    REQUIRE_SUCCESS(callPowerUpdateClaimedPower(delta));
    return outcome::success();
  }

  outcome::result<ChainEpoch> MinerUtils::assignProvingPeriodOffset(
      ChainEpoch current_epoch) const {
    OUTCOME_TRY(address_encoded,
                codec::cbor::encode(getRuntime().getCurrentReceiver()));
    address_encoded.putUint64(current_epoch);
    OUTCOME_TRY(digest, getRuntime().hashBlake2b(address_encoded));
    const uint64_t offset = boost::endian::load_big_u64(digest.data());
    return offset % kWPoStProvingPeriod;
  }

  ChainEpoch MinerUtils::nextProvingPeriodStart(ChainEpoch current_epoch,
                                                ChainEpoch offset) const {
    const auto current_modulus = current_epoch % kWPoStProvingPeriod;
    const ChainEpoch period_progress =
        current_modulus >= offset
            ? current_modulus - offset
            : kWPoStProvingPeriod - (offset - current_modulus);
    const ChainEpoch period_start =
        current_epoch - period_progress + kWPoStProvingPeriod;
    return period_start;
  }

  ChainEpoch MinerUtils::currentProvingPeriodStart(ChainEpoch current_epoch,
                                                   ChainEpoch offset) const {
    // Do nothing for v0
    return 0;
  }

  outcome::result<void> MinerUtils::validateExpiration(
      ChainEpoch activation,
      ChainEpoch expiration,
      RegisteredSealProof seal_proof) const {
    OUTCOME_TRY(getRuntime().validateArgument(expiration - activation
                                              >= kMinSectorExpiration));
    OUTCOME_TRY(getRuntime().validateArgument(
        expiration
        <= getRuntime().getCurrentEpoch() + kMaxSectorExpirationExtension));

    const Universal<ProofPolicy> proof_policy{getRuntime().getActorVersion()};
    REQUIRE_NO_ERROR_A(max_lifetime,
                       proof_policy->getSealProofSectorMaximumLifetime(
                           seal_proof, getRuntime().getNetworkVersion()),
                       VMExitCode::kErrIllegalArgument);
    OUTCOME_TRY(
        getRuntime().validateArgument(expiration - activation <= max_lifetime));
    return outcome::success();
  }

  outcome::result<SectorOnChainInfo> MinerUtils::validateReplaceSector(
      MinerActorStatePtr &state, const SectorPreCommitInfo &params) const {
    CHANGE_ERROR_A(replace_sector,
                   state->sectors.sectors.get(params.replace_sector),
                   VMExitCode::kErrNotFound);

    OUTCOME_TRY(getRuntime().validateArgument(replace_sector.deals.empty()));

    OUTCOME_TRY(getRuntime().validateArgument(params.registered_proof
                                              == replace_sector.seal_proof));
    OUTCOME_TRY(getRuntime().validateArgument(params.expiration
                                              >= replace_sector.expiration));

    REQUIRE_NO_ERROR(state->checkSectorHealth(params.replace_deadline,
                                              params.replace_partition,
                                              params.replace_sector),
                     VMExitCode::kErrIllegalState);

    return std::move(replace_sector);
  }

  outcome::result<uint64_t> MinerUtils::currentDeadlineIndex(
      ChainEpoch current_epoch, ChainEpoch period_start) const {
    // Do nothing for v0
    return 0;
  }

  outcome::result<void> MinerUtils::canPreCommitSealProof(
      RegisteredSealProof seal_proof_type,
      NetworkVersion network_version) const {
    // Do nothing for v0
    return outcome::success();
  }

  outcome::result<void> MinerUtils::checkPeerInfo(
      const Buffer &peer_id,
      const std::vector<Multiaddress> &multiaddresses) const {
    // Do nothing for v0
    return outcome::success();
  }

  outcome::result<void> MinerUtils::checkControlAddresses(
      const std::vector<Address> &control_addresses) const {
    // Do nothing for v0
    return outcome::success();
  }

  outcome::result<EpochReward> MinerUtils::requestCurrentEpochBlockReward()
      const {
    REQUIRE_SUCCESS_A(
        reward,
        getRuntime().sendM<reward::ThisEpochReward>(kRewardAddress, {}, 0));
    return EpochReward{
        .this_epoch_reward = reward.this_epoch_reward,
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

  outcome::result<DealWeights> MinerUtils::requestDealWeight(
      const std::vector<DealId> &deals,
      ChainEpoch sector_start,
      ChainEpoch sector_expiry) const {
    REQUIRE_SUCCESS_A(deal_weights,
                      getRuntime().sendM<market::VerifyDealsForActivation>(
                          kStorageMarketAddress,
                          {.deals = deals,
                           .sector_expiry = sector_expiry,
                           .sector_start = sector_start},
                          0));
    return DealWeights{
        .deal_weight = deal_weights.deal_weight,
        .verified_deal_weight = deal_weights.verified_deal_weight,
        .deal_space = 0};
  }

  outcome::result<void> MinerUtils::verifyWindowedPost(
      ChainEpoch challenge_epoch,
      const std::vector<SectorOnChainInfo> &sectors,
      const std::vector<PoStProof> &proofs) const {
    const auto miner_actor_id = getRuntime().getCurrentReceiver().getId();

    OUTCOME_TRY(addr_buf,
                codec::cbor::encode(getRuntime().getCurrentReceiver()));
    OUTCOME_TRY(post_randomness,
                getRuntime().getRandomnessFromBeacon(
                    DomainSeparationTag::WindowedPoStChallengeSeed,
                    challenge_epoch,
                    addr_buf));

    std::vector<SectorInfo> sector_proof_info;
    for (const auto &sector : sectors) {
      sector_proof_info.push_back({.registered_proof = sector.seal_proof,
                                   .sector = sector.sector,
                                   .sealed_cid = sector.sealed_cid});
    }

    const WindowPoStVerifyInfo post_verify_info{
        .randomness = post_randomness,
        .proofs = proofs,
        .challenged_sectors = sector_proof_info,
        .prover = miner_actor_id};

    OUTCOME_TRY(verified, getRuntime().verifyPoSt(post_verify_info));
    OUTCOME_TRY(getRuntime().validateArgument(verified));

    return outcome::success();
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

}  // namespace fc::vm::actor::builtin::v0::miner
