/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/miner/miner_actor.hpp"

#include "vm/actor/builtin/states/miner/miner_actor_state.hpp"
#include "vm/actor/builtin/types/type_manager/type_manager.hpp"
#include "vm/actor/builtin/v3/account/account_actor.hpp"
#include "vm/actor/builtin/v3/storage_power/storage_power_actor_export.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::actor::builtin::v3::miner {
  using states::MinerActorStatePtr;
  using toolchain::Toolchain;
  using types::TypeManager;
  using namespace types::miner;

  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kInitAddress));

    const auto utils = Toolchain::createMinerUtils(runtime);

    OUTCOME_TRY(utils->checkControlAddresses(params.control_addresses));
    OUTCOME_TRY(utils->checkPeerInfo(params.peer_id, params.multiaddresses));
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
    OUTCOME_TRY(runtime.requireState(period_start <= current_epoch));
    state->proving_period_start = period_start;

    OUTCOME_TRY(deadline_index,
                utils->currentDeadlineIndex(current_epoch, period_start));
    OUTCOME_TRY(runtime.requireState(deadline_index < kWPoStPeriodDeadlines));
    state->current_deadline = deadline_index;

    REQUIRE_NO_ERROR_A(
        miner_info,
        TypeManager::makeMinerInfo(runtime,
                                   owner,
                                   worker,
                                   control_addresses,
                                   params.peer_id,
                                   params.multiaddresses,
                                   RegisteredSealProof::kUndefined,
                                   params.post_proof_type),
        VMExitCode::kErrIllegalState);
    OUTCOME_TRY(state->miner_info.set(miner_info));

    OUTCOME_TRY(runtime.commitState(state));

    const ChainEpoch deadline_close =
        period_start + kWPoStChallengeWindow * (1 + deadline_index);
    OUTCOME_TRY(utils->enrollCronEvent(deadline_close - 1,
                                       {CronEventType::kProvingDeadline}));

    return outcome::success();
  }

  ACTOR_METHOD_IMPL(DisputeWindowedPoSt) {
    // TODO (a.chernyshov) FIL-342 implement
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
      exportMethod<DisputeWindowedPoSt>(),
  };

}  // namespace fc::vm::actor::builtin::v3::miner
