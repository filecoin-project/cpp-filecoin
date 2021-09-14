/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/miner/miner_actor_utils.hpp"

#include <boost/endian/conversion.hpp>
#include "vm/actor/builtin/v0/account/account_actor.hpp"
#include "vm/actor/builtin/v0/storage_power/storage_power_actor.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::actor::builtin::v0::miner {
  using toolchain::Toolchain;
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

}  // namespace fc::vm::actor::builtin::v0::miner
