/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/miner/miner_actor.hpp"

#include "vm/actor/builtin/states/miner_actor_state.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"
#include "vm/actor/builtin/types/type_manager/type_manager.hpp"
#include "vm/actor/builtin/v2/account/account_actor.hpp"
#include "vm/actor/builtin/v2/storage_power/storage_power_actor_export.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  using primitives::ChainEpoch;
  using primitives::RleBitset;
  using states::MinerActorStatePtr;
  using toolchain::Toolchain;
  using types::TypeManager;
  using namespace types::miner;

  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kInitAddress));

    const auto utils = Toolchain::createMinerUtils(runtime);

    OUTCOME_TRY(utils->checkControlAddresses(params.control_addresses));
    OUTCOME_TRY(utils->checkPeerInfo(params.peer_id, params.multiaddresses));
    CHANGE_ERROR_ABORT(utils->canPreCommitSealProof(
                           params.seal_proof_type, runtime.getNetworkVersion()),
                       VMExitCode::kErrIllegalArgument);
    OUTCOME_TRY(owner, utils->resolveControlAddress(params.owner));
    OUTCOME_TRY(worker, utils->resolveWorkerAddress(params.worker));
    std::vector<Address> control_addresses;
    for (const auto &control_address : params.control_addresses) {
      OUTCOME_TRY(resolved, utils->resolveControlAddress(control_address));
      control_addresses.push_back(resolved);
    }

    MinerActorStatePtr state{runtime.getActorVersion()};
    cbor_blake::cbLoadT(runtime.getIpfsDatastore(), state);
    OUTCOME_TRY(v0::miner::Construct::makeEmptyState(runtime, state));

    const auto current_epoch = runtime.getCurrentEpoch();
    REQUIRE_NO_ERROR_A(offset,
                       utils->assignProvingPeriodOffset(current_epoch),
                       VMExitCode::kErrSerialization);
    const auto period_start =
        utils->currentProvingPeriodStart(current_epoch, offset);
    VM_ASSERT(period_start <= current_epoch);
    state->proving_period_start = period_start;

    OUTCOME_TRY(deadline_index,
                utils->currentDeadlineIndex(current_epoch, period_start));
    VM_ASSERT(deadline_index < kWPoStPeriodDeadlines);
    state->current_deadline = deadline_index;

    REQUIRE_NO_ERROR_A(
        miner_info,
        TypeManager::makeMinerInfo(runtime,
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

    const ChainEpoch deadline_close =
        period_start + kWPoStChallengeWindow * (1 + deadline_index);
    OUTCOME_TRY(utils->enrollCronEvent(deadline_close - 1,
                                       {CronEventType::kProvingDeadline}));

    return outcome::success();
  }

  ACTOR_METHOD_IMPL(ChangeWorkerAddress) {
    const auto utils = Toolchain::createMinerUtils(runtime);

    OUTCOME_TRY(utils->checkControlAddresses(params.new_control_addresses));

    OUTCOME_TRY(new_worker, utils->resolveWorkerAddress(params.new_worker));

    std::vector<Address> control_addresses;
    for (const auto &address : params.new_control_addresses) {
      OUTCOME_TRY(resolved, utils->resolveControlAddress(address));
      control_addresses.emplace_back(resolved);
    }

    OUTCOME_TRY(state, runtime.getActorState<MinerActorStatePtr>());
    OUTCOME_TRY(miner_info, state->getInfo());

    OUTCOME_TRY(runtime.validateImmediateCallerIs(miner_info->owner));

    miner_info->control = control_addresses;

    if ((new_worker != miner_info->worker) && !miner_info->pending_worker_key) {
      miner_info->pending_worker_key = WorkerKeyChange{
          .new_worker = new_worker,
          .effective_at = runtime.getCurrentEpoch() + kWorkerKeyChangeDelay};
    }

    REQUIRE_NO_ERROR(state->miner_info.set(miner_info),
                     VMExitCode::kErrIllegalState);
    OUTCOME_TRY(runtime.commitState(state));

    return outcome::success();
  }

  ACTOR_METHOD_IMPL(ApplyRewards) {
    // TODO (a.chernyshov) FIL-310 - implement
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(ConfirmUpdateWorkerKey) {
    // TODO (a.chernyshov) FIL-317 implement
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(RepayDebt) {
    // TODO (a.chernyshov) FIL-318 implement
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(ChangeOwnerAddress) {
    // TODO (a.chernyshov) FIL-319 implement
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
      exportMethod<ApplyRewards>(),
      exportMethod<ReportConsensusFault>(),
      exportMethod<WithdrawBalance>(),
      exportMethod<ConfirmSectorProofsValid>(),
      exportMethod<ChangeMultiaddresses>(),
      exportMethod<CompactPartitions>(),
      exportMethod<CompactSectorNumbers>(),
      exportMethod<ConfirmUpdateWorkerKey>(),
      exportMethod<RepayDebt>(),
      exportMethod<ChangeOwnerAddress>(),
  };

}  // namespace fc::vm::actor::builtin::v2::miner
