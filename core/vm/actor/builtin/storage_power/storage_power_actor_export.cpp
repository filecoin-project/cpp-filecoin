/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/storage_power/storage_power_actor_export.hpp"

#include "vm/actor/builtin/init/init_actor.hpp"
#include "vm/actor/builtin/miner/miner_actor.hpp"
#include "vm/actor/builtin/shared/shared.hpp"
#include "vm/actor/builtin/storage_power/storage_power_actor_state.hpp"

namespace fc::vm::actor::builtin::storage_power {

  using primitives::SectorStorageWeightDesc;
  using primitives::TokenAmount;
  using primitives::address::decode;
  using vm::actor::ActorExports;
  using vm::actor::kConstructorMethodNumber;
  using vm::actor::builtin::requestMinerControlAddress;
  using vm::runtime::InvocationOutput;
  using vm::runtime::Runtime;

  outcome::result<void> assertImmediateCallerTypeIsMiner(Runtime &runtime) {
    OUTCOME_TRY(immediate_caller_code_id,
                runtime.getActorCodeID(runtime.getImmediateCaller()));
    if (immediate_caller_code_id != kStorageMinerCodeCid)
      return VMExitCode::STORAGE_POWER_ACTOR_WRONG_CALLER;
    return outcome::success();
  }

  /**
   * Get current storage power actor state
   * @param runtime - current runtime
   * @return current storage power actor state or appropriate error
   */
  outcome::result<StoragePowerActor> getCurrentState(Runtime &runtime) {
    auto datastore = runtime.getIpfsDatastore();
    OUTCOME_TRY(state,
                runtime.getCurrentActorStateCbor<StoragePowerActorState>());
    return StoragePowerActor(datastore, state);
  }

  outcome::result<InvocationOutput> slashPledgeCollateral(
      Runtime &runtime,
      StoragePowerActor &power_actor,
      Address miner,
      TokenAmount to_slash) {
    OUTCOME_TRY(
        slashed,
        power_actor.subtractMinerBalance(miner, to_slash, TokenAmount{0}));
    OUTCOME_TRY(runtime.sendFunds(kBurntFundsActorAddress, slashed));

    return outcome::success();
  }

  /**
   * Deletes miner from state and slashes miner balance
   * @param runtime - current runtime
   * @param state - current storage power actor state
   * @param miner address to delete
   * @return error in case of failure
   */
  outcome::result<void> deleteMinerActor(Runtime &runtime,
                                         StoragePowerActor &state,
                                         const Address &miner) {
    OUTCOME_TRY(amount_slashed, state.deleteMiner(miner));
    OUTCOME_TRY(runtime.send(
        miner, miner::kOnDeleteMinerMethodNumber, {}, TokenAmount{0}));
    OUTCOME_TRY(runtime.sendFunds(kBurntFundsActorAddress, amount_slashed));
    return outcome::success();
  }

  ACTOR_METHOD(StoragePowerActorMethods::construct) {
    if (runtime.getImmediateCaller() != kSystemActorAddress)
      return VMExitCode::STORAGE_POWER_ACTOR_WRONG_CALLER;

    auto datastore = runtime.getIpfsDatastore();
    OUTCOME_TRY(empty_state, StoragePowerActor::createEmptyState(datastore));

    OUTCOME_TRY(runtime.commitState(empty_state));
    return outcome::success();
  }

  ACTOR_METHOD(StoragePowerActorMethods::addBalance) {
    OUTCOME_TRY(add_balance_params,
                decodeActorParams<AddBalanceParameters>(params));
    OUTCOME_TRY(miner_code_cid,
                runtime.getActorCodeID(add_balance_params.miner));
    if (miner_code_cid != kStorageMinerCodeCid)
      return VMExitCode::STORAGE_POWER_ILLEGAL_ARGUMENT;

    Address immediate_caller = runtime.getImmediateCaller();
    OUTCOME_TRY(control_addresses,
                requestMinerControlAddress(runtime, add_balance_params.miner));
    if (immediate_caller != control_addresses.owner
        && immediate_caller != control_addresses.worker)
      return VMExitCode::STORAGE_POWER_FORBIDDEN;

    OUTCOME_TRY(power_actor, getCurrentState(runtime));
    OUTCOME_TRY(power_actor.addMinerBalance(add_balance_params.miner,
                                            runtime.getMessage().get().value));

    OUTCOME_TRY(power_actor_state, power_actor.flushState());
    OUTCOME_TRY(runtime.commitState(power_actor_state));
    return outcome::success();
  }

  ACTOR_METHOD(StoragePowerActorMethods::withdrawBalance) {
    OUTCOME_TRY(withdraw_balance_params,
                decodeActorParams<WithdrawBalanceParameters>(params));
    OUTCOME_TRY(miner_code_cid,
                runtime.getActorCodeID(withdraw_balance_params.miner));
    if (miner_code_cid != kStorageMinerCodeCid)
      return VMExitCode::STORAGE_POWER_ILLEGAL_ARGUMENT;

    Address immediate_caller = runtime.getImmediateCaller();
    OUTCOME_TRY(
        control_addresses,
        requestMinerControlAddress(runtime, withdraw_balance_params.miner));
    if (immediate_caller != control_addresses.owner
        && immediate_caller != control_addresses.worker)
      return VMExitCode::STORAGE_POWER_FORBIDDEN;

    if (withdraw_balance_params.requested < TokenAmount{0})
      return VMExitCode::STORAGE_POWER_ILLEGAL_ARGUMENT;

    OUTCOME_TRY(power_actor, getCurrentState(runtime));

    if (!power_actor.hasClaim(withdraw_balance_params.miner))
      return VMExitCode::STORAGE_POWER_ILLEGAL_ARGUMENT;
    OUTCOME_TRY(claim, power_actor.getClaim(withdraw_balance_params.miner));

    /*
     * Pledge for sectors in temporary fault has already been subtracted from
     * the claim. If the miner has failed a scheduled PoSt, collateral remains
     * locked for further penalization. Thus the current claimed pledge is the
     * amount to keep locked.
     */
    OUTCOME_TRY(
        subtracted,
        power_actor.subtractMinerBalance(withdraw_balance_params.miner,
                                         withdraw_balance_params.requested,
                                         claim.pledge));

    OUTCOME_TRY(runtime.sendFunds(control_addresses.owner, subtracted));

    OUTCOME_TRY(power_actor_state, power_actor.flushState());
    OUTCOME_TRY(runtime.commitState(power_actor_state));
    return outcome::success();
  }

  ACTOR_METHOD(StoragePowerActorMethods::createMiner) {
    OUTCOME_TRY(immediate_caller_code_id,
                runtime.getActorCodeID(runtime.getImmediateCaller()));
    if (!isSignableActor((immediate_caller_code_id)))
      return VMExitCode::STORAGE_POWER_FORBIDDEN;

    OUTCOME_TRY(create_miner_params,
                decodeActorParams<CreateMinerParameters>(params));

    auto message = runtime.getMessage().get();
    miner::ConstructorParams construct_miner_parameters{
        message.from,
        create_miner_params.worker,
        create_miner_params.sector_size,
        create_miner_params.peer_id};
    OUTCOME_TRY(encoded_construct_miner_parameters,
                encodeActorParams(construct_miner_parameters));
    OUTCOME_TRY(addresses_created,
                runtime.sendPR<init::ExecReturn>(
                    kInitAddress,
                    init::kExecMethodNumber,
                    init::ExecParams{
                        .code = kStorageMinerCodeCid,
                        .params = encoded_construct_miner_parameters,
                    },
                    TokenAmount{0}));

    OUTCOME_TRY(power_actor, getCurrentState(runtime));
    OUTCOME_TRY(power_actor.addMiner(addresses_created.id_address));
    OUTCOME_TRY(power_actor.setMinerBalance(addresses_created.id_address,
                                            message.value));

    CreateMinerReturn result{addresses_created.id_address,
                             addresses_created.robust_address};
    OUTCOME_TRY(output, encodeActorReturn(result));

    OUTCOME_TRY(power_actor_state, power_actor.flushState());
    OUTCOME_TRY(runtime.commitState(power_actor_state));
    return std::move(output);
  }

  ACTOR_METHOD(StoragePowerActorMethods::deleteMiner) {
    OUTCOME_TRY(delete_miner_params,
                decodeActorParams<DeleteMinerParameters>(params));

    Address immediate_caller = runtime.getImmediateCaller();
    OUTCOME_TRY(control_addresses,
                requestMinerControlAddress(runtime, delete_miner_params.miner));
    if (immediate_caller != control_addresses.owner
        && immediate_caller != control_addresses.worker)
      return VMExitCode::STORAGE_POWER_FORBIDDEN;

    OUTCOME_TRY(power_actor, getCurrentState(runtime));

    OUTCOME_TRY(
        deleteMinerActor(runtime, power_actor, delete_miner_params.miner));

    OUTCOME_TRY(power_actor_state, power_actor.flushState());
    OUTCOME_TRY(runtime.commitState(power_actor_state));
    return outcome::success();
  }

  ACTOR_METHOD(StoragePowerActorMethods::onSectorProveCommit) {
    OUTCOME_TRY(assertImmediateCallerTypeIsMiner(runtime));
    OUTCOME_TRY(on_sector_prove_commit_params,
                decodeActorParams<OnSectorProveCommitParameters>(params));

    OUTCOME_TRY(power_actor, getCurrentState(runtime));

    StoragePower power =
        consensusPowerForWeight(on_sector_prove_commit_params.weight);
    OUTCOME_TRY(network_power, power_actor.getTotalNetworkPower());
    TokenAmount pledge =
        pledgeForWeight(on_sector_prove_commit_params.weight, network_power);
    OUTCOME_TRY(
        power_actor.addToClaim(runtime.getMessage().get().from, power, pledge));

    OUTCOME_TRY(result,
                encodeActorReturn(OnSectorProveCommitReturn{.pledge = pledge}));

    OUTCOME_TRY(power_actor_state, power_actor.flushState());
    OUTCOME_TRY(runtime.commitState(power_actor_state));

    return std::move(result);
  }

  ACTOR_METHOD(StoragePowerActorMethods::onSectorTerminate) {
    OUTCOME_TRY(assertImmediateCallerTypeIsMiner(runtime));
    OUTCOME_TRY(on_sector_terminate_params,
                decodeActorParams<OnSectorTerminateParameters>(params));

    Address miner_address = runtime.getMessage().get().from;
    StoragePower power =
        consensusPowerForWeights(on_sector_terminate_params.weights);

    OUTCOME_TRY(power_actor, getCurrentState(runtime));

    OUTCOME_TRY(power_actor.addToClaim(
        miner_address, -power, -on_sector_terminate_params.pledge));

    if (on_sector_terminate_params.termination_type
        != SectorTerminationType::SECTOR_TERMINATION_EXPIRED) {
      TokenAmount amount_to_slash = pledgePenaltyForSectorTermination(
          on_sector_terminate_params.pledge,
          on_sector_terminate_params.termination_type);
      OUTCOME_TRY(slashPledgeCollateral(
          runtime, power_actor, miner_address, amount_to_slash));
    }

    OUTCOME_TRY(power_actor_state, power_actor.flushState());
    OUTCOME_TRY(runtime.commitState(power_actor_state));
    return outcome::success();
  }

  ACTOR_METHOD(StoragePowerActorMethods::onSectorTemporaryFaultEffectiveBegin) {
    OUTCOME_TRY(assertImmediateCallerTypeIsMiner(runtime));
    OUTCOME_TRY(
        on_sector_fault_params,
        decodeActorParams<OnSectorTemporaryFaultEffectiveBeginParameters>(
            params));
    StoragePower power =
        consensusPowerForWeights(on_sector_fault_params.weights);

    OUTCOME_TRY(power_actor, getCurrentState(runtime));

    OUTCOME_TRY(power_actor.addToClaim(runtime.getMessage().get().from,
                                       -power,
                                       -on_sector_fault_params.pledge));

    OUTCOME_TRY(power_actor_state, power_actor.flushState());
    OUTCOME_TRY(runtime.commitState(power_actor_state));
    return outcome::success();
  }

  ACTOR_METHOD(StoragePowerActorMethods::onSectorTemporaryFaultEffectiveEnd) {
    OUTCOME_TRY(assertImmediateCallerTypeIsMiner(runtime));
    OUTCOME_TRY(on_sector_fault_end_params,
                decodeActorParams<OnSectorTemporaryFaultEffectiveEndParameters>(
                    params));
    StoragePower power =
        consensusPowerForWeights(on_sector_fault_end_params.weights);
    OUTCOME_TRY(power_actor, getCurrentState(runtime));
    OUTCOME_TRY(power_actor.addToClaim(runtime.getMessage().get().from,
                                       power,
                                       on_sector_fault_end_params.pledge));

    OUTCOME_TRY(power_actor_state, power_actor.flushState());
    OUTCOME_TRY(runtime.commitState(power_actor_state));
    return outcome::success();
  }

  ACTOR_METHOD(StoragePowerActorMethods::onSectorModifyWeightDesc) {
    OUTCOME_TRY(assertImmediateCallerTypeIsMiner(runtime));
    OUTCOME_TRY(on_sector_modify_params,
                decodeActorParams<OnSectorModifyWeightDescParameters>(params));

    OUTCOME_TRY(power_actor, getCurrentState(runtime));
    Address miner = runtime.getMessage().get().from;

    StoragePower prev_power =
        consensusPowerForWeight(on_sector_modify_params.prev_weight);
    OUTCOME_TRY(power_actor.addToClaim(
        miner, -prev_power, -on_sector_modify_params.prev_pledge));

    StoragePower new_power =
        consensusPowerForWeight(on_sector_modify_params.new_weight);
    OUTCOME_TRY(total_power, power_actor.getTotalNetworkPower());
    TokenAmount new_pledge =
        pledgeForWeight(on_sector_modify_params.new_weight, total_power);
    OUTCOME_TRY(power_actor.addToClaim(miner, new_power, new_pledge));

    OUTCOME_TRY(result,
                encodeActorReturn(
                    OnSectorModifyWeightDescReturn{.new_pledge = new_pledge}));

    OUTCOME_TRY(power_actor_state, power_actor.flushState());
    OUTCOME_TRY(runtime.commitState(power_actor_state));
    return std::move(result);
  }

  ACTOR_METHOD(StoragePowerActorMethods::onMinerWindowedPoStSuccess) {
    OUTCOME_TRY(assertImmediateCallerTypeIsMiner(runtime));

    OUTCOME_TRY(power_actor, getCurrentState(runtime));
    Address miner = runtime.getMessage().get().from;
    OUTCOME_TRY(fault, power_actor.hasFaultMiner(miner));
    if (fault) {
      OUTCOME_TRY(power_actor.deleteFaultMiner(miner));
    }

    OUTCOME_TRY(power_actor_state, power_actor.flushState());
    OUTCOME_TRY(runtime.commitState(power_actor_state));
    return outcome::success();
  }

  ACTOR_METHOD(StoragePowerActorMethods::onMinerWindowedPoStFailure) {
    OUTCOME_TRY(assertImmediateCallerTypeIsMiner(runtime));
    Address miner = runtime.getMessage().get().from;
    OUTCOME_TRY(power_actor, getCurrentState(runtime));
    OUTCOME_TRY(power_actor.addFaultMiner(miner));

    OUTCOME_TRY(
        parameters,
        decodeActorParams<OnMinerWindowedPoStFailureParameters>(params));
    if (parameters.num_consecutive_failures > kWindowedPostFailureLimit) {
      OUTCOME_TRY(deleteMinerActor(runtime, power_actor, miner));
    } else {
      OUTCOME_TRY(claim, power_actor.getClaim(miner));
      TokenAmount to_slash = pledgePenaltyForWindowedPoStFailure(
          claim.pledge, parameters.num_consecutive_failures);
      OUTCOME_TRY(slashPledgeCollateral(runtime, power_actor, miner, to_slash));
    }

    OUTCOME_TRY(power_actor_state, power_actor.flushState());
    OUTCOME_TRY(runtime.commitState(power_actor_state));
    return outcome::success();
  }

  ACTOR_METHOD(StoragePowerActorMethods::enrollCronEvent) {
    OUTCOME_TRY(assertImmediateCallerTypeIsMiner(runtime));
    Address miner = runtime.getMessage().get().from;
    OUTCOME_TRY(power_actor, getCurrentState(runtime));
    OUTCOME_TRY(parameters,
                decodeActorParams<EnrollCronEventParameters>(params));
    OUTCOME_TRY(power_actor.appendCronEvent(
        parameters.event_epoch,
        CronEvent{.miner_address = miner,
                  .callback_payload = parameters.payload}));

    OUTCOME_TRY(power_actor_state, power_actor.flushState());
    OUTCOME_TRY(runtime.commitState(power_actor_state));
    return outcome::success();
  }

  ACTOR_METHOD(StoragePowerActorMethods::reportConsensusFault) {
    // Note: only the first reporter of any fault is rewarded.
    // Subsequent invocations fail because the miner has been removed.
    OUTCOME_TRY(parameters,
                decodeActorParams<ReportConsensusFaultParameters>(params));
    OUTCOME_TRY(fault,
                runtime.verifyConsensusFault(parameters.block_header_1,
                                             parameters.block_header_2));
    if (!fault) {
      return VMExitCode::STORAGE_POWER_ILLEGAL_ARGUMENT;
    }

    Address reporter = runtime.getMessage().get().from;
    OUTCOME_TRY(target, runtime.resolveAddress(parameters.target));
    OUTCOME_TRY(power_actor, getCurrentState(runtime));
    OUTCOME_TRY(claim, power_actor.getClaim(target));
    if (claim.power < 0) {
      return VMExitCode::STORAGE_POWER_ILLEGAL_STATE;
    }
    OUTCOME_TRY(balance, power_actor.getMinerBalance(target));
    // elapsed epoch from the latter block which committed the fault
    ChainEpoch elapsed = runtime.getCurrentEpoch() - parameters.fault_epoch;
    if (elapsed < 0) {
      return VMExitCode::STORAGE_POWER_ILLEGAL_ARGUMENT;
    }
    OUTCOME_TRY(collateral_to_slash,
                pledgePenaltyForConsensusFault(balance, parameters.fault_type));
    TokenAmount target_reward =
        rewardForConsensusSlashReport(elapsed, collateral_to_slash);
    OUTCOME_TRY(reward,
                power_actor.subtractMinerBalance(
                    target, target_reward, TokenAmount{0}));
    OUTCOME_TRY(runtime.sendFunds(reporter, reward));
    OUTCOME_TRY(deleteMinerActor(runtime, power_actor, target));

    OUTCOME_TRY(power_actor_state, power_actor.flushState());
    OUTCOME_TRY(runtime.commitState(power_actor_state));
    return outcome::success();
  }

  ACTOR_METHOD(StoragePowerActorMethods::onEpochTickEnd) {
    if (runtime.getImmediateCaller() != kCronAddress)
      return VMExitCode::STORAGE_POWER_ACTOR_WRONG_CALLER;

    ChainEpoch epoch = runtime.getCurrentEpoch();
    OUTCOME_TRY(power_actor, getCurrentState(runtime));
    OUTCOME_TRY(events, power_actor.getCronEvents(epoch));
    OUTCOME_TRY(power_actor.clearCronEvents(epoch));
    for (const auto event : events) {
      OUTCOME_TRY(runtime.send(event.miner_address,
                               miner::kOnDeferredCronEventMethodNumber,
                               MethodParams{event.callback_payload},
                               TokenAmount{0}));
    }

    OUTCOME_TRY(power_actor_state, power_actor.flushState());
    OUTCOME_TRY(runtime.commitState(power_actor_state));
    return outcome::success();
  }

  const ActorExports exports = {
      {kConstructorMethodNumber,
       ActorMethod(StoragePowerActorMethods::construct)},
      {kAddBalanceMethodNumber,
       ActorMethod(StoragePowerActorMethods::addBalance)},
      {kWithdrawBalanceMethodNumber,
       ActorMethod(StoragePowerActorMethods::withdrawBalance)},
      {kCreateMinerMethodNumber,
       ActorMethod(StoragePowerActorMethods::createMiner)},
      {kDeleteMinerMethodNumber,
       ActorMethod(StoragePowerActorMethods::deleteMiner)},
      {kOnSectorProveCommitMethodNumber,
       ActorMethod(StoragePowerActorMethods::onSectorProveCommit)},
      {kOnSectorTerminateMethodNumber,
       ActorMethod(StoragePowerActorMethods::onSectorTerminate)},
      {kOnSectorTerminateMethodNumber,
       ActorMethod(
           StoragePowerActorMethods::onSectorTemporaryFaultEffectiveBegin)},
      {kOnSectorTemporaryFaultEffectiveEndMethodNumber,
       ActorMethod(
           StoragePowerActorMethods::onSectorTemporaryFaultEffectiveEnd)},
      {kOnSectorModifyWeightDescMethodNumber,
       ActorMethod(StoragePowerActorMethods::onSectorModifyWeightDesc)},
      {kOnMinerWindowedPoStSuccessMethodNumber,
       ActorMethod(StoragePowerActorMethods::onMinerWindowedPoStSuccess)},
      {kOnMinerWindowedPoStSuccessMethodNumber,
       ActorMethod(StoragePowerActorMethods::onMinerWindowedPoStFailure)},
      {kEnrollCronEventMethodNumber,
       ActorMethod(StoragePowerActorMethods::enrollCronEvent)},
      {kReportConsensusFaultMethodNumber,
       ActorMethod(StoragePowerActorMethods::reportConsensusFault)},
      {kOnEpochTickEndMethodNumber,
       ActorMethod(StoragePowerActorMethods::onEpochTickEnd)}};

}  // namespace fc::vm::actor::builtin::storage_power
