/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/miner/miner_actor.hpp"

#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::actor::builtin::v0::miner {
  using toolchain::Toolchain;
  using namespace types::miner;

  outcome::result<void> Construct::makeEmptyState(const Runtime &runtime,
                                                  MinerActorStatePtr &state) {
    // Lotus gas conformance - flush empty hamt
    OUTCOME_TRY(state->precommitted_sectors.hamt.flush());

    // Lotus gas conformance - flush empty hamt
    OUTCOME_TRY(empty_amt_cid, state->precommitted_setctors_expiry.amt.flush());

    RleBitset allocated_sectors;
    OUTCOME_TRY(state->allocated_sectors.set(allocated_sectors));

    OUTCOME_TRY(deadlines, makeEmptyDeadlines(runtime, empty_amt_cid));
    OUTCOME_TRY(state->deadlines.set(deadlines));

    VestingFunds vesting_funds;
    OUTCOME_TRY(state->vesting_funds.set(vesting_funds));

    // construct with empty already cid stored in ipld to avoid gas charge
    state->sectors = {empty_amt_cid, runtime};

    return outcome::success();
  }

  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kInitAddress));

    // proof is supported
    OUTCOME_TRY(
        runtime.validateArgument(kSupportedProofs.find(params.seal_proof_type)
                                 != kSupportedProofs.end()));

    const auto utils = Toolchain::createMinerUtils(runtime);

    OUTCOME_TRY(owner, utils->resolveControlAddress(params.owner));
    OUTCOME_TRY(worker, utils->resolveWorkerAddress(params.worker));
    std::vector<Address> control_addresses;
    for (const auto &control_address : params.control_addresses) {
      OUTCOME_TRY(resolved, utils->resolveControlAddress(control_address));
      control_addresses.push_back(resolved);
    }

    const auto actor_version = runtime.getActorVersion();

    MinerActorStatePtr state{actor_version};
    cbor_blake::cbLoadT(runtime.getIpfsDatastore(), state);
    OUTCOME_TRY(makeEmptyState(runtime, state));

    const auto current_epoch = runtime.getCurrentEpoch();
    REQUIRE_NO_ERROR_A(offset,
                       utils->assignProvingPeriodOffset(current_epoch),
                       VMExitCode::kErrSerialization);
    const auto period_start =
        utils->nextProvingPeriodStart(current_epoch, offset);
    VM_ASSERT(period_start > current_epoch);
    state->proving_period_start = period_start;

    REQUIRE_NO_ERROR_A(miner_info,
                       makeMinerInfo(actor_version,
                                     owner,
                                     worker,
                                     control_addresses,
                                     params.peer_id,
                                     params.multiaddresses,
                                     params.seal_proof_type,
                                     RegisteredPoStProof::kUndefined),
                       VMExitCode::kErrIllegalArgument);
    OUTCOME_TRY(state->miner_info.set(miner_info));

    OUTCOME_TRY(runtime.commitState(state));

    OUTCOME_TRY(utils->enrollCronEvent(period_start - 1,
                                       {CronEventType::kProvingDeadline}));

    return outcome::success();
  }

  ACTOR_METHOD_IMPL(ControlAddresses) {
    OUTCOME_TRY(state, runtime.getActorState<MinerActorStatePtr>());
    REQUIRE_NO_ERROR_A(
        miner_info, state->getInfo(), VMExitCode::kErrIllegalState);
    return Result{.owner = miner_info->owner,
                  .worker = miner_info->worker,
                  .control = miner_info->control};
  }

  ACTOR_METHOD_IMPL(ChangeWorkerAddress) {
    ChainEpoch effective_epoch{};

    const auto utils = Toolchain::createMinerUtils(runtime);

    OUTCOME_TRY(new_worker, utils->resolveWorkerAddress(params.new_worker));

    std::vector<Address> control_addresses;
    for (const auto &address : params.new_control_addresses) {
      OUTCOME_TRY(resolved, utils->resolveControlAddress(address));
      control_addresses.emplace_back(resolved);
    }

    bool worker_changed = false;

    OUTCOME_TRY(state, runtime.getActorState<MinerActorStatePtr>());
    OUTCOME_TRY(miner_info, state->getInfo());

    OUTCOME_TRY(runtime.validateImmediateCallerIs(miner_info->owner));

    miner_info->control = control_addresses;

    if (new_worker != miner_info->worker) {
      worker_changed = true;
      effective_epoch = runtime.getCurrentEpoch() + kWorkerKeyChangeDelay;

      miner_info->pending_worker_key = WorkerKeyChange{
          .new_worker = new_worker, .effective_at = effective_epoch};
    }

    REQUIRE_NO_ERROR(state->miner_info.set(miner_info),
                     VMExitCode::kErrIllegalState);
    OUTCOME_TRY(runtime.commitState(state));

    if (worker_changed) {
      const CronEventPayload cron_payload{.event_type =
                                              CronEventType::kWorkerKeyChange};
      OUTCOME_TRY(utils->enrollCronEvent(effective_epoch, cron_payload));
    }

    return outcome::success();
  }

  ACTOR_METHOD_IMPL(ChangePeerId) {
    const auto utils = Toolchain::createMinerUtils(runtime);

    OUTCOME_TRY(utils->checkPeerInfo(params.new_id, {}));

    OUTCOME_TRY(state, runtime.getActorState<MinerActorStatePtr>());

    OUTCOME_TRY(miner_info, state->getInfo());

    auto callers = miner_info->control;
    callers.emplace_back(miner_info->owner);
    callers.emplace_back(miner_info->worker);
    OUTCOME_TRY(runtime.validateImmediateCallerIs(callers));

    miner_info->peer_id = params.new_id;
    REQUIRE_NO_ERROR(state->miner_info.set(miner_info),
                     VMExitCode::kErrIllegalState);

    OUTCOME_TRY(runtime.commitState(state));

    return outcome::success();
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
