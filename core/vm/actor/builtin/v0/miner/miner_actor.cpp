/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/miner/miner_actor.hpp"

#include "vm/actor/builtin/types/miner/monies.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::actor::builtin::v0::miner {
  using crypto::randomness::DomainSeparationTag;
  using states::makeEmptyMinerState;
  using states::MinerActorStatePtr;
  using toolchain::Toolchain;
  using types::Universal;
  using namespace types::miner;

  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kInitAddress));

    // proof is supported
    VALIDATE_ARG(kSupportedProofs.find(params.seal_proof_type)
                 != kSupportedProofs.end());

    const auto utils = Toolchain::createMinerUtils(runtime);

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
        utils->nextProvingPeriodStart(current_epoch, offset);
    VM_ASSERT(period_start > current_epoch);
    state->proving_period_start = period_start;

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
    const auto current_epoch = runtime.getCurrentEpoch();
    const auto network_version = runtime.getNetworkVersion();

    VALIDATE_ARG(params.deadline < kWPoStPeriodDeadlines);
    VALIDATE_ARG(params.chain_commit_epoch < current_epoch);
    VALIDATE_ARG(params.chain_commit_epoch
                 >= current_epoch - kWPoStChallengeWindow);

    OUTCOME_TRY(
        randomness,
        runtime.getRandomnessFromTickets(DomainSeparationTag::PoStChainCommit,
                                         params.chain_commit_epoch,
                                         {}));
    VALIDATE_ARG(randomness == params.chain_commit_rand);

    const auto utils = Toolchain::createMinerUtils(runtime);

    OUTCOME_TRY(reward, utils->requestCurrentEpochBlockReward());
    OUTCOME_TRY(total_power, utils->requestCurrentTotalPower());

    OUTCOME_TRY(state, runtime.getActorState<MinerActorStatePtr>());

    OUTCOME_TRY(miner_info, state->getInfo());

    auto callers = miner_info->control;
    callers.emplace_back(miner_info->owner);
    callers.emplace_back(miner_info->worker);
    OUTCOME_TRY(runtime.validateImmediateCallerIs(callers));

    const auto submission_partition_limit = utils->loadPartitionsSectorsMax(
        miner_info->window_post_partition_sectors);
    VALIDATE_ARG(params.partitions.size() <= submission_partition_limit);

    const auto deadline_info = state->deadlineInfo(current_epoch);
    REQUIRE_NO_ERROR_A(
        deadlines, state->deadlines.get(), VMExitCode::kErrIllegalState);

    if (!deadline_info.isOpen()) {
      ABORT(VMExitCode::kErrIllegalState);
    }

    VALIDATE_ARG(params.deadline == deadline_info.index);

    REQUIRE_NO_ERROR_A(
        sectors, state->sectors.loadSectors(), VMExitCode::kErrIllegalState);

    REQUIRE_NO_ERROR_A(deadline,
                       deadlines.loadDeadline(params.deadline),
                       VMExitCode::kErrIllegalState);

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
        loadSectorInfosForProof(
            state->sectors, post_result.sectors, post_result.ignored_sectors),
        VMExitCode::kErrIllegalState);

    if (!sector_infos.empty()) {
      OUTCOME_TRY(utils->verifyWindowedPost(
          deadline_info.challenge, sector_infos, params.proofs));
    }

    const auto undeclared_penalty_power = post_result.penaltyPower();
    TokenAmount undeclared_penalty_target{0};
    TokenAmount declared_penalty_target{0};

    if (network_version < NetworkVersion::kVersion3) {
      const Universal<Monies> monies{runtime.getActorVersion()};

      OUTCOME_TRYA(undeclared_penalty_target,
                   monies->pledgePenaltyForUndeclaredFault(
                       reward.this_epoch_reward_smoothed,
                       total_power.quality_adj_power_smoothed,
                       undeclared_penalty_power.qa,
                       network_version));

      OUTCOME_TRY(deducted,
                  monies->pledgePenaltyForDeclaredFault(
                      reward.this_epoch_reward_smoothed,
                      total_power.quality_adj_power_smoothed,
                      undeclared_penalty_power.qa,
                      network_version));

      undeclared_penalty_target -= deducted;

      OUTCOME_TRYA(declared_penalty_target,
                   monies->pledgePenaltyForDeclaredFault(
                       reward.this_epoch_reward_smoothed,
                       total_power.quality_adj_power_smoothed,
                       post_result.recovered_power.qa,
                       network_version));
    }

    const TokenAmount total_penalty_target =
        undeclared_penalty_target + declared_penalty_target;
    OUTCOME_TRY(actor_balance, runtime.getCurrentBalance());
    OUTCOME_TRY(unlocked_balance, state->getUnlockedBalance(actor_balance));
    REQUIRE_NO_ERROR_A(
        penalty_result,
        state->penalizeFundsInPriorityOrder(
            current_epoch, total_penalty_target, unlocked_balance),
        VMExitCode::kErrIllegalState);
    const auto &[vesting_penalty_total, balance_penalty_total] = penalty_result;
    const TokenAmount penalty_total =
        vesting_penalty_total + balance_penalty_total;
    const TokenAmount pledge_delta = -vesting_penalty_total;

    REQUIRE_NO_ERROR(deadlines.updateDeadline(params.deadline, deadline),
                     VMExitCode::kErrIllegalState);

    REQUIRE_NO_ERROR(state->deadlines.set(deadlines),
                     VMExitCode::kErrIllegalState);

    OUTCOME_TRY(runtime.commitState(state));

    OUTCOME_TRY(utils->requestUpdatePower(post_result.powerDelta()));

    if (penalty_total > 0) {
      REQUIRE_SUCCESS(
          runtime.sendFunds(kBurntFundsActorAddress, penalty_total));
    }

    OUTCOME_TRY(utils->notifyPledgeChanged(pledge_delta));

    return outcome::success();
  }

  ACTOR_METHOD_IMPL(PreCommitSector) {
    const auto current_epoch = runtime.getCurrentEpoch();

    const auto utils = Toolchain::createMinerUtils(runtime);

    OUTCOME_TRY(utils->canPreCommitSealProof(params.registered_proof,
                                             runtime.getNetworkVersion()));

    VALIDATE_ARG(params.sector <= kMaxSectorNumber);

    VALIDATE_ARG(params.sealed_cid != CID());

    VALIDATE_ARG(params.sealed_cid.getPrefix() == kSealedCIDPrefix);

    VALIDATE_ARG(params.seal_epoch < current_epoch);

    OUTCOME_TRY(max_seal_duration, maxSealDuration(params.registered_proof));
    const auto challenge_earliest =
        current_epoch - kChainFinality - max_seal_duration;

    VALIDATE_ARG(params.seal_epoch >= challenge_earliest);

    VALIDATE_ARG(params.expiration > current_epoch);

    VALIDATE_ARG(!(params.replace_capacity && params.deal_ids.empty()));

    VALIDATE_ARG(params.replace_deadline < kWPoStPeriodDeadlines);

    VALIDATE_ARG(params.replace_sector <= kMaxSectorNumber);

    OUTCOME_TRY(reward, utils->requestCurrentEpochBlockReward());
    OUTCOME_TRY(total_power, utils->requestCurrentTotalPower());
    OUTCOME_TRY(deal_weight,
                utils->requestDealWeight(
                    params.deal_ids, current_epoch, params.expiration));

    OUTCOME_TRY(state, runtime.getActorState<MinerActorStatePtr>());

    OUTCOME_TRY(miner_info, state->getInfo());

    auto callers = miner_info->control;
    callers.emplace_back(miner_info->owner);
    callers.emplace_back(miner_info->worker);
    OUTCOME_TRY(runtime.validateImmediateCallerIs(callers));

    VALIDATE_ARG(params.registered_proof == miner_info->seal_proof_type);

    VALIDATE_ARG(params.deal_ids.size()
                 <= sectorDealsMax(miner_info->sector_size));

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

    const auto max_activation = current_epoch + max_seal_duration;
    OUTCOME_TRY(utils->validateExpiration(
        max_activation, params.expiration, params.registered_proof));

    TokenAmount deposit_minimum = 0;
    if (params.replace_capacity) {
      OUTCOME_TRY(replace_sector, utils->validateReplaceSector(state, params));
      deposit_minimum = replace_sector->init_pledge;
    }

    REQUIRE_NO_ERROR_A(newly_vested,
                       state->unlockVestedFunds(current_epoch),
                       VMExitCode::kErrIllegalState);
    OUTCOME_TRY(current_balance, runtime.getCurrentBalance());
    OUTCOME_TRY(available_balance, state->getAvailableBalance(current_balance));
    const auto duration = params.expiration - current_epoch;

    const auto sector_weight =
        qaPowerForWeight(miner_info->sector_size,
                         duration,
                         deal_weight.deal_weight,
                         deal_weight.verified_deal_weight);
    const Universal<Monies> monies{runtime.getActorVersion()};
    OUTCOME_TRY(
        pre_commit_deposit,
        monies->preCommitDepositForPower(reward.this_epoch_reward_smoothed,
                                         total_power.quality_adj_power_smoothed,
                                         sector_weight));
    const auto deposit_req = std::max(pre_commit_deposit, deposit_minimum);

    if (available_balance < deposit_req) {
      ABORT(VMExitCode::kErrInsufficientFunds);
    }

    OUTCOME_TRY(state->addPreCommitDeposit(deposit_req));
    OUTCOME_TRY(state->checkBalanceInvariants(current_balance));

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

    const auto expiry_bound = current_epoch + max_seal_duration + 1;

    REQUIRE_NO_ERROR(state->addPreCommitExpiry(expiry_bound, params.sector),
                     VMExitCode::kErrIllegalState);

    OUTCOME_TRY(runtime.commitState(state));

    OUTCOME_TRY(utils->notifyPledgeChanged(-newly_vested));

    return outcome::success();
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
