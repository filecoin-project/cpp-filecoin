/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/miner/miner_actor.hpp"

#include "common/be_decoder.hpp"
#include "vm/actor/builtin/v0/account/account_actor.hpp"
#include "vm/actor/builtin/v0/codes.hpp"
#include "vm/actor/builtin/v0/miner/miner_actor_state.hpp"
#include "vm/actor/builtin/v0/storage_power/storage_power_actor_export.hpp"

namespace fc::vm::actor::builtin::v0::miner {
  using common::decodeBE;

  /**
   * Resolves an address to an ID address and verifies that it is address of an
   * account or multisig actor
   * @param address to resolve
   * @return resolved address
   */
  outcome::result<Address> resolveControlAddress(Runtime &runtime,
                                                 const Address &address) {
    OUTCOME_TRY(resolved, runtime.resolveAddress(address));
    VM_ASSERT(resolved.isId());
    const auto resolved_code = runtime.getActorCodeID(resolved);
    REQUIRE_NO_ERROR(resolved_code, VMExitCode::kErrIllegalArgument);
    OUTCOME_TRY(
        runtime.validateArgument(isSignableActor(resolved_code.value())));
    return std::move(resolved);
  }

  /**
   * Resolves an address to an ID address and verifies that it is address of an
   * account actor with an associated BLS key. The worker must be BLS since the
   * worker key will be used alongside a BLS-VRF.
   * @param runtime
   * @param address to resolve
   * @return resolved address
   */
  outcome::result<Address> resolveWorkerAddress(Runtime &runtime,
                                                const Address &address) {
    OUTCOME_TRY(resolved, runtime.resolveAddress(address));
    VM_ASSERT(resolved.isId());
    const auto resolved_code = runtime.getActorCodeID(resolved);
    REQUIRE_NO_ERROR(resolved_code, VMExitCode::kErrIllegalArgument);
    OUTCOME_TRY(
        runtime.validateArgument(resolved_code.value() == kAccountCodeCid));

    if (!address.isBls()) {
      OUTCOME_TRY(pubkey_addres,
                  runtime.sendM<account::PubkeyAddress>(resolved, {}, 0));
      OUTCOME_TRY(runtime.validateArgument(pubkey_addres.isBls()));
    }

    return std::move(resolved);
  }

  /**
   * Registers first cron callback for epoch before the first proving period
   * starts.
   */
  outcome::result<void> enrollCronEvent(Runtime &runtime,
                                        ChainEpoch event_epoch,
                                        const CronEventPayload &payload) {
    OUTCOME_TRY(encoded_params, codec::cbor::encode(payload));
    OUTCOME_TRY(runtime.sendM<storage_power::EnrollCronEvent>(
        kStoragePowerAddress, {event_epoch, encoded_params}, 0));
    return outcome::success();
  }

  outcome::result<ChainEpoch> Construct::assignProvingPeriodOffset(
      Runtime &runtime, ChainEpoch current_epoch) {
    OUTCOME_TRY(address_encoded,
                codec::cbor::encode(runtime.getCurrentReceiver()));
    address_encoded.putUint64(current_epoch);
    OUTCOME_TRY(digest, runtime.hashBlake2b(address_encoded));
    const uint64_t offset = decodeBE(digest);
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

    State state;

    // Lotus gas conformance - flush empty hamt
    state.precommitted_sectors = {};
    runtime.getIpfsDatastore()->load(state);
    OUTCOME_TRY(state.precommitted_sectors.hamt.flush());

    // Lotus gas conformance - flush empty hamt
    state.precommitted_setctors_expiry = {};
    runtime.getIpfsDatastore()->load(state);

    OUTCOME_TRY(empty_amt_cid, state.precommitted_setctors_expiry.amt.flush());

    RleBitset allocated_sectors;
    OUTCOME_TRY(state.allocated_sectors.set(allocated_sectors));

    OUTCOME_TRY(
        deadlines,
        Deadlines::makeEmpty(runtime.getIpfsDatastore(), empty_amt_cid));
    OUTCOME_TRY(state.deadlines.set(deadlines));

    VestingFunds vesting_funds;
    OUTCOME_TRY(state.vesting_funds.set(vesting_funds));

    const auto current_epoch = runtime.getCurrentEpoch();
    const auto offset = assignProvingPeriodOffset(runtime, current_epoch);
    REQUIRE_NO_ERROR(offset, VMExitCode::kErrSerialization);
    const auto period_start =
        nextProvingPeriodStart(current_epoch, offset.value());
    VM_ASSERT(period_start > current_epoch);
    state.proving_period_start = period_start;

    OUTCOME_TRY(miner_info,
                MinerInfo::make(owner,
                                worker,
                                control_addresses,
                                params.peer_id,
                                params.multiaddresses,
                                params.seal_proof_type));
    OUTCOME_TRY(state.info.set(miner_info));

    // construct with empty already cid stored in ipld to avoid gas charge
    state.sectors = adt::Array<SectorOnChainInfo>(empty_amt_cid,
                                                  runtime.getIpfsDatastore());

    OUTCOME_TRY(runtime.commitState(state));

    OUTCOME_TRY(enrollCronEvent(
        runtime, period_start - 1, {CronEventType::kProvingDeadline}));

    return outcome::success();
  }

  ACTOR_METHOD_IMPL(ControlAddresses) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(ChangeWorkerAddress) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(ChangePeerId) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(SubmitWindowedPoSt) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(PreCommitSector) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(ProveCommitSector) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(ExtendSectorExpiration) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(TerminateSectors) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(DeclareFaults) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(DeclareFaultsRecovered) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(OnDeferredCronEvent) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(CheckSectorProven) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(AddLockedFund) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(ReportConsensusFault) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(WithdrawBalance) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(ConfirmSectorProofsValid) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(ChangeMultiaddresses) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(CompactPartitions) {
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(CompactSectorNumbers) {
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
