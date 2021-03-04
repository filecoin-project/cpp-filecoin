/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/miner/miner_actor.hpp"

#include <boost/endian/conversion.hpp>

#include "vm/actor/builtin/v0/account/account_actor.hpp"
#include "vm/actor/builtin/v0/storage_power/storage_power_actor_export.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::actor::builtin::v0::miner {
  using toolchain::Toolchain;
  using namespace types::miner;

  outcome::result<Address> resolveControlAddress(const Runtime &runtime,
                                                 const Address &address) {
    const auto resolved = runtime.resolveAddress(address);
    OUTCOME_TRY(runtime.validateArgument(!resolved.has_error()));
    VM_ASSERT(resolved.value().isId());
    const auto resolved_code = runtime.getActorCodeID(resolved.value());
    OUTCOME_TRY(runtime.validateArgument(!resolved_code.has_error()));

    const auto address_matcher =
        Toolchain::createAddressMatcher(runtime.getActorVersion());
    OUTCOME_TRY(runtime.validateArgument(
        address_matcher->isSignableActor(resolved_code.value())));
    return std::move(resolved.value());
  }

  outcome::result<Address> resolveWorkerAddress(Runtime &runtime,
                                                const Address &address) {
    const auto resolved = runtime.resolveAddress(address);
    OUTCOME_TRY(runtime.validateArgument(!resolved.has_error()));
    VM_ASSERT(resolved.value().isId());
    const auto resolved_code = runtime.getActorCodeID(resolved.value());
    OUTCOME_TRY(runtime.validateArgument(!resolved_code.has_error()));
    const auto address_matcher =
        Toolchain::createAddressMatcher(runtime.getActorVersion());
    OUTCOME_TRY(runtime.validateArgument(
        resolved_code.value() == address_matcher->getAccountCodeId()));

    if (!address.isBls()) {
      const auto pubkey_addres =
          runtime.sendM<account::PubkeyAddress>(resolved.value(), {}, 0);
      OUTCOME_TRY(runtime.validateArgument(pubkey_addres.value().isBls()));
    }

    return std::move(resolved.value());
  }

  outcome::result<void> enrollCronEvent(Runtime &runtime,
                                        ChainEpoch event_epoch,
                                        const CronEventPayload &payload) {
    const auto encoded_params = codec::cbor::encode(payload);
    OUTCOME_TRY(runtime.validateArgument(!encoded_params.has_error()));
    REQUIRE_SUCCESS(runtime.sendM<storage_power::EnrollCronEvent>(
        kStoragePowerAddress, {event_epoch, encoded_params.value()}, 0));
    return outcome::success();
  }

  outcome::result<ChainEpoch> Construct::assignProvingPeriodOffset(
      Runtime &runtime, ChainEpoch current_epoch) {
    OUTCOME_TRY(address_encoded,
                codec::cbor::encode(runtime.getCurrentReceiver()));
    address_encoded.putUint64(current_epoch);
    OUTCOME_TRY(digest, runtime.hashBlake2b(address_encoded));
    const uint64_t offset = boost::endian::load_big_u64(digest.data());
    return offset % kWPoStProvingPeriod;
  }

  ChainEpoch Construct::nextProvingPeriodStart(ChainEpoch current_epoch,
                                               ChainEpoch offset) {
    const auto current_modulus = current_epoch % kWPoStProvingPeriod;
    const ChainEpoch period_progress =
        current_modulus >= offset
            ? current_modulus - offset
            : kWPoStProvingPeriod - (offset - current_modulus);
    const ChainEpoch period_start =
        current_epoch - period_progress + kWPoStProvingPeriod;
    return period_start;
  }

  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kInitAddress));

    // proof is supported
    OUTCOME_TRY(
        runtime.validateArgument(kSupportedProofs.find(params.seal_proof_type)
                                 != kSupportedProofs.end()));
    OUTCOME_TRY(owner, resolveControlAddress(runtime, params.owner));
    OUTCOME_TRY(worker, resolveWorkerAddress(runtime, params.worker));
    std::vector<Address> control_addresses;
    for (const auto &control_address : params.control_addresses) {
      OUTCOME_TRY(resolved, resolveControlAddress(runtime, control_address));
      control_addresses.push_back(resolved);
    }

    auto state = runtime.stateManager()->createMinerActorState(
        runtime.getActorVersion());

    // Lotus gas conformance - flush empty hamt
    OUTCOME_TRY(state->precommitted_sectors.hamt.flush());

    // Lotus gas conformance - flush empty hamt
    OUTCOME_TRY(empty_amt_cid, state->precommitted_setctors_expiry.amt.flush());

    RleBitset allocated_sectors;
    OUTCOME_TRY(state->allocated_sectors.set(allocated_sectors));

    OUTCOME_TRY(
        deadlines,
        state->makeEmptyDeadlines(runtime.getIpfsDatastore(), empty_amt_cid));
    OUTCOME_TRY(state->deadlines.set(deadlines));

    VestingFunds vesting_funds;
    OUTCOME_TRY(state->vesting_funds.set(vesting_funds));

    const auto current_epoch = runtime.getCurrentEpoch();
    REQUIRE_NO_ERROR_A(offset,
                       assignProvingPeriodOffset(runtime, current_epoch),
                       VMExitCode::kErrSerialization);
    const auto period_start = nextProvingPeriodStart(current_epoch, offset);
    VM_ASSERT(period_start > current_epoch);
    state->proving_period_start = period_start;

    REQUIRE_NO_ERROR_A(miner_info,
                       MinerInfo::make(owner,
                                       worker,
                                       control_addresses,
                                       params.peer_id,
                                       params.multiaddresses,
                                       params.seal_proof_type),
                       VMExitCode::kErrIllegalArgument);
    OUTCOME_TRY(state->setInfo(runtime.getIpfsDatastore(), miner_info));

    // construct with empty already cid stored in ipld to avoid gas charge
    state->sectors = adt::Array<SectorOnChainInfo>(empty_amt_cid,
                                                   runtime.getIpfsDatastore());

    OUTCOME_TRY(runtime.commitState(state));

    OUTCOME_TRY(enrollCronEvent(
        runtime, period_start - 1, {CronEventType::kProvingDeadline}));

    return outcome::success();
  }

  ACTOR_METHOD_IMPL(ControlAddresses) {
    // TODO (a.chernyshov) FIL-279 - implement
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(ChangeWorkerAddress) {
    // TODO (a.chernyshov) FIL-280 - implement
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(ChangePeerId) {
    // TODO (a.chernyshov) FIL-281 - implement
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(SubmitWindowedPoSt) {
    // TODO (a.chernyshov) FIL-282 - implement
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(PreCommitSector) {
    // TODO (a.chernyshov) FIL-283 - implement
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(ProveCommitSector) {
    // TODO (a.chernyshov) FIL-284 - implement
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(ExtendSectorExpiration) {
    // TODO (a.chernyshov) FIL-285 - implement
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(TerminateSectors) {
    // TODO (a.chernyshov) FIL-286 - implement
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(DeclareFaults) {
    // TODO (a.chernyshov) FIL-287 - implement
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(DeclareFaultsRecovered) {
    // TODO (a.chernyshov) FIL-288 - implement
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(OnDeferredCronEvent) {
    // TODO (a.chernyshov) FIL-289 - implement
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(CheckSectorProven) {
    // TODO (a.chernyshov) FIL-290 - implement
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(AddLockedFund) {
    // TODO (a.chernyshov) FIL-291 - implement
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(ReportConsensusFault) {
    // TODO (a.chernyshov) FIL-292 - implement
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(WithdrawBalance) {
    // TODO (a.chernyshov) FIL-293 - implement
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(ConfirmSectorProofsValid) {
    // TODO (a.chernyshov) FIL-294 - implement
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(ChangeMultiaddresses) {
    // TODO (a.chernyshov) FIL-295 - implement
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(CompactPartitions) {
    // TODO (a.chernyshov) FIL-296 - implement
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(CompactSectorNumbers) {
    // TODO (a.chernyshov) FIL-297 - implement
    return VMExitCode::kNotImplemented;
  }

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<ControlAddresses>(),
      exportMethod<ChangeWorkerAddress>(),
      exportMethod<ChangePeerId>(),
      exportMethod<SubmitWindowedPoSt>(),
      exportMethod<PreCommitSector>(),
      exportMethod<ProveCommitSector>(),
      exportMethod<ExtendSectorExpiration>(),
      exportMethod<TerminateSectors>(),
      exportMethod<DeclareFaults>(),
      exportMethod<DeclareFaultsRecovered>(),
      exportMethod<OnDeferredCronEvent>(),
      exportMethod<CheckSectorProven>(),
      exportMethod<AddLockedFund>(),
      exportMethod<ReportConsensusFault>(),
      exportMethod<WithdrawBalance>(),
      exportMethod<ConfirmSectorProofsValid>(),
      exportMethod<ChangeMultiaddresses>(),
      exportMethod<CompactPartitions>(),
      exportMethod<CompactSectorNumbers>(),
  };
}  // namespace fc::vm::actor::builtin::v0::miner
