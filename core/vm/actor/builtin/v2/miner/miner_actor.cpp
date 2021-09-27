/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/miner/miner_actor.hpp"

#include "vm/actor/builtin/states/miner/miner_actor_state.hpp"
#include "vm/actor/builtin/types/miner/monies.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  using crypto::randomness::DomainSeparationTag;
  using crypto::randomness::kRandomnessLength;
  using crypto::randomness::Randomness;
  using primitives::ChainEpoch;
  using primitives::RleBitset;
  using states::makeEmptyMinerState;
  using states::MinerActorStatePtr;
  using toolchain::Toolchain;
  using types::Universal;
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

    OUTCOME_TRY(state, makeEmptyMinerState(runtime));

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

    REQUIRE_NO_ERROR_A(miner_info,
                       makeMinerInfo(runtime.getActorVersion(),
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

  ACTOR_METHOD_IMPL(SubmitWindowedPoSt) {
    const auto current_epoch = runtime.getCurrentEpoch();
    const auto network_version = runtime.getNetworkVersion();

    OUTCOME_TRY(
        runtime.validateArgument(params.deadline < kWPoStPeriodDeadlines));

    OUTCOME_TRY(runtime.validateArgument(params.chain_commit_rand.size()
                                         <= kRandomnessLength));

    RleBitset partition_indexes;
    if (network_version >= NetworkVersion::kVersion7) {
      for (const auto &partition : params.partitions) {
        partition_indexes.insert(partition.index);
      }
    }

    const auto utils = Toolchain::createMinerUtils(runtime);

    OUTCOME_TRY(state, runtime.getActorState<MinerActorStatePtr>());

    OUTCOME_TRY(miner_info, state->getInfo());

    auto callers = miner_info->control;
    callers.emplace_back(miner_info->owner);
    callers.emplace_back(miner_info->worker);
    OUTCOME_TRY(runtime.validateImmediateCallerIs(callers));

    OUTCOME_TRY(runtime.validateArgument(params.proofs.size() == 1));
    OUTCOME_TRY(
        runtime.validateArgument(params.proofs[0].registered_proof
                                 == miner_info->window_post_proof_type));

    const auto submission_partition_limit = utils->loadPartitionsSectorsMax(
        miner_info->window_post_partition_sectors);
    OUTCOME_TRY(runtime.validateArgument(params.partitions.size()
                                         <= submission_partition_limit));

    const auto deadline_info = state->deadlineInfo(current_epoch);

    if (!deadline_info.isOpen()) {
      ABORT(VMExitCode::kErrIllegalState);
    }

    OUTCOME_TRY(
        runtime.validateArgument(params.deadline == deadline_info.index));

    OUTCOME_TRY(runtime.validateArgument(params.chain_commit_epoch
                                         >= deadline_info.challenge));

    OUTCOME_TRY(
        runtime.validateArgument(params.chain_commit_epoch < current_epoch));

    OUTCOME_TRY(
        randomness,
        runtime.getRandomnessFromTickets(DomainSeparationTag::PoStChainCommit,
                                         params.chain_commit_epoch,
                                         {}));
    OUTCOME_TRY(
        runtime.validateArgument(randomness == params.chain_commit_rand));

    REQUIRE_NO_ERROR_A(
        sectors, state->sectors.loadSectors(), VMExitCode::kErrIllegalState);

    REQUIRE_NO_ERROR_A(
        deadlines, state->deadlines.get(), VMExitCode::kErrIllegalState);

    REQUIRE_NO_ERROR_A(deadline,
                       deadlines.loadDeadline(params.deadline),
                       VMExitCode::kErrIllegalState);

    if (network_version >= NetworkVersion::kVersion7) {
      const auto already_proven =
          deadline->partitions_posted.intersect(partition_indexes);
      OUTCOME_TRY(runtime.validateArgument(already_proven.empty()));
    }

    const auto fault_expiration = deadline_info.last() + kFaultMaxAge;
    REQUIRE_NO_ERROR_A(post_result,
                       deadline->recordProvenSectors(runtime,
                                                     sectors,
                                                     miner_info->sector_size,
                                                     deadline_info.quant(),
                                                     fault_expiration,
                                                     params.partitions),
                       VMExitCode::kErrIllegalState);

    REQUIRE_NO_ERROR_A(
        sector_infos,
        sectors.loadForProof(post_result.sectors, post_result.ignored_sectors),
        VMExitCode::kErrIllegalState);

    OUTCOME_TRY(runtime.validateArgument(!sector_infos.empty()));

    OUTCOME_TRY(utils->verifyWindowedPost(
        deadline_info.challenge, sector_infos, params.proofs));

    REQUIRE_NO_ERROR(deadlines.updateDeadline(params.deadline, deadline),
                     VMExitCode::kErrIllegalState);

    REQUIRE_NO_ERROR(state->deadlines.set(deadlines),
                     VMExitCode::kErrIllegalState);

    OUTCOME_TRY(runtime.commitState(state));

    OUTCOME_TRY(utils->requestUpdatePower(post_result.power_delta));

    OUTCOME_TRYA(state, runtime.getActorState<MinerActorStatePtr>());

    OUTCOME_TRY(balance, runtime.getCurrentBalance());
    REQUIRE_NO_ERROR(state->checkBalanceInvariants(balance),
                     VMExitCode::kErrIllegalState);

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
