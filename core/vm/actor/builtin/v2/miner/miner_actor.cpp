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
    OUTCOME_TRY(utils->canPreCommitSealProof(params.seal_proof_type,
                                             runtime.getNetworkVersion()));
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

    VALIDATE_ARG(params.deadline < kWPoStPeriodDeadlines);

    VALIDATE_ARG(params.chain_commit_rand.size() <= kRandomnessLength);

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

    VALIDATE_ARG(params.proofs.size() == 1);
    VALIDATE_ARG(params.proofs[0].registered_proof
                 == miner_info->window_post_proof_type);

    const auto submission_partition_limit = utils->loadPartitionsSectorsMax(
        miner_info->window_post_partition_sectors);
    VALIDATE_ARG(params.partitions.size() <= submission_partition_limit);

    const auto deadline_info = state->deadlineInfo(current_epoch);

    if (!deadline_info.isOpen()) {
      ABORT(VMExitCode::kErrIllegalState);
    }

    VALIDATE_ARG(params.deadline == deadline_info.index);

    VALIDATE_ARG(params.chain_commit_epoch >= deadline_info.challenge);

    VALIDATE_ARG(params.chain_commit_epoch < current_epoch);

    OUTCOME_TRY(
        randomness,
        runtime.getRandomnessFromTickets(DomainSeparationTag::PoStChainCommit,
                                         params.chain_commit_epoch,
                                         {}));
    VALIDATE_ARG(randomness == params.chain_commit_rand);

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
      VALIDATE_ARG(already_proven.empty());
    }

    const auto fault_expiration = deadline_info.last() + kFaultMaxAge;
    REQUIRE_NO_ERROR_A(post_result,
                       deadline->recordProvenSectors(sectors,
                                                     miner_info->sector_size,
                                                     deadline_info.quant(),
                                                     fault_expiration,
                                                     params.partitions),
                       VMExitCode::kErrIllegalState);

    REQUIRE_NO_ERROR_A(
        sector_infos,
        sectors.loadForProof(post_result.sectors, post_result.ignored_sectors),
        VMExitCode::kErrIllegalState);

    VALIDATE_ARG(!sector_infos.empty());

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

  ACTOR_METHOD_IMPL(PreCommitSector) {
    const auto current_epoch = runtime.getCurrentEpoch();
    const auto network_version = runtime.getNetworkVersion();

    const auto utils = Toolchain::createMinerUtils(runtime);

    OUTCOME_TRY(
        utils->canPreCommitSealProof(params.registered_proof, network_version));

    VALIDATE_ARG(params.sector <= kMaxSectorNumber);

    VALIDATE_ARG(params.sealed_cid != CID());

    VALIDATE_ARG(params.sealed_cid.getPrefix() == kSealedCIDPrefix);

    VALIDATE_ARG(params.seal_epoch < current_epoch);

    const auto challenge_earliest =
        current_epoch - kMaxPreCommitRandomnessLookback;

    VALIDATE_ARG(params.seal_epoch >= challenge_earliest);

    OUTCOME_TRY(max_seal_duration, maxSealDuration(params.registered_proof));
    const auto max_activation = current_epoch + max_seal_duration;
    OUTCOME_TRY(utils->validateExpiration(
        max_activation, params.expiration, params.registered_proof));

    VALIDATE_ARG(!(params.replace_capacity && params.deal_ids.empty()));

    VALIDATE_ARG(params.replace_deadline < kWPoStPeriodDeadlines);

    VALIDATE_ARG(params.replace_sector <= kMaxSectorNumber);

    OUTCOME_TRY(reward, utils->requestCurrentEpochBlockReward());
    OUTCOME_TRY(total_power, utils->requestCurrentTotalPower());
    OUTCOME_TRY(deal_weight,
                utils->requestDealWeight(
                    params.deal_ids, current_epoch, params.expiration));

    OUTCOME_TRY(state, runtime.getActorState<MinerActorStatePtr>());

    TokenAmount newly_vested{0};
    if (network_version < NetworkVersion::kVersion7) {
      REQUIRE_NO_ERROR_A(vested,
                         state->unlockVestedFunds(current_epoch),
                         VMExitCode::kErrIllegalState);
      newly_vested = vested;
    }

    OUTCOME_TRY(current_balance, runtime.getCurrentBalance());
    REQUIRE_NO_ERROR_A(available_balance,
                       state->getAvailableBalance(current_balance),
                       VMExitCode::kErrIllegalState);

    const Universal<Monies> monies{runtime.getActorVersion()};
    OUTCOME_TRY(fee_to_burn, monies->repayDebtsOrAbort(runtime, state));

    OUTCOME_TRY(miner_info, state->getInfo());

    auto callers = miner_info->control;
    callers.emplace_back(miner_info->owner);
    callers.emplace_back(miner_info->worker);
    OUTCOME_TRY(runtime.validateImmediateCallerIs(callers));

    if (current_epoch <= miner_info->consensus_fault_elapsed) {
      ABORT(VMExitCode::kErrForbidden);
    }

    if (network_version < NetworkVersion::kVersion7) {
      VALIDATE_ARG(params.registered_proof == miner_info->seal_proof_type);
    } else {
      REQUIRE_NO_ERROR_A(sector_wpost_proof,
                         getRegisteredWindowPoStProof(params.registered_proof),
                         VMExitCode::kErrIllegalArgument);
      VALIDATE_ARG(sector_wpost_proof == miner_info->window_post_proof_type);
    }

    VALIDATE_ARG(params.deal_ids.size()
                 <= sectorDealsMax(miner_info->sector_size));

    VALIDATE_ARG(deal_weight.deal_space <= miner_info->sector_size);

    REQUIRE_NO_ERROR(state->allocateSectorNumber(params.sector),
                     VMExitCode::kErrIllegalState);

    // Lotus gas conformance
    const auto precommitted_sectors_copy = state->precommitted_sectors;
    REQUIRE_NO_ERROR_A(precommit_found,
                       precommitted_sectors_copy.has(params.sector),
                       VMExitCode::kErrIllegalState);
    VALIDATE_ARG(!precommit_found);

    REQUIRE_NO_ERROR_A(
        sectors, state->sectors.loadSectors(), VMExitCode::kErrIllegalState);
    REQUIRE_NO_ERROR_A(sector_found,
                       sectors.sectors.has(params.sector),
                       VMExitCode::kErrIllegalState);
    VALIDATE_ARG(!sector_found);

    if (params.replace_capacity) {
      OUTCOME_TRY(utils->validateReplaceSector(state, params));
    }

    const auto duration = params.expiration - current_epoch;

    const auto sector_weight =
        qaPowerForWeight(miner_info->sector_size,
                         duration,
                         deal_weight.deal_weight,
                         deal_weight.verified_deal_weight);

    OUTCOME_TRY(
        deposit_req,
        monies->preCommitDepositForPower(reward.this_epoch_reward_smoothed,
                                         total_power.quality_adj_power_smoothed,
                                         sector_weight));

    if (available_balance < deposit_req) {
      ABORT(VMExitCode::kErrInsufficientFunds);
    }

    OUTCOME_TRY(state->addPreCommitDeposit(deposit_req));

    const SectorPreCommitOnChainInfo sector_precommit_info{
        .info = params,
        .precommit_deposit = deposit_req,
        .precommit_epoch = current_epoch,
        .deal_weight = deal_weight.deal_weight,
        .verified_deal_weight = deal_weight.verified_deal_weight};
    CHANGE_ERROR(
        state->precommitted_sectors.set(params.sector, sector_precommit_info),
        VMExitCode::kErrIllegalState);

    // Lotus gas conformance
    OUTCOME_TRY(state->precommitted_sectors.hamt.flush());

    const auto expiry_bound = current_epoch + kMaxProveCommitDuration + 1;

    REQUIRE_NO_ERROR(state->addPreCommitExpiry(expiry_bound, params.sector),
                     VMExitCode::kErrIllegalState);

    OUTCOME_TRY(runtime.commitState(state));

    if (fee_to_burn > 0) {
      REQUIRE_SUCCESS(runtime.sendFunds(kBurntFundsActorAddress, fee_to_burn));
    }

    // Lotus gas conformance
    OUTCOME_TRYA(state, runtime.getActorState<MinerActorStatePtr>());

    REQUIRE_NO_ERROR(state->checkBalanceInvariants(current_balance),
                     VMExitCode::kErrBalanceInvariantBroken);

    OUTCOME_TRY(utils->notifyPledgeChanged(-newly_vested));

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
