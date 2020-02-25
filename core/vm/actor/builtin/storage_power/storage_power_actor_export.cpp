/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/storage_power/storage_power_actor_export.hpp"

#include "vm/actor/builtin/init/init_actor.hpp"
#include "vm/actor/builtin/miner/miner_actor.hpp"
#include "vm/actor/builtin/shared/shared.hpp"

using fc::primitives::SectorStorageWeightDesc;
using fc::primitives::TokenAmount;
using fc::primitives::address::decode;
using fc::vm::actor::ActorExports;
using fc::vm::actor::kConstructorMethodNumber;
using fc::vm::actor::builtin::requestMinerControlAddress;
using fc::vm::actor::builtin::storage_power::kEpochTotalExpectedReward;
using fc::vm::actor::builtin::storage_power::kPledgeFactor;
using fc::vm::actor::builtin::storage_power::StoragePower;
using fc::vm::actor::builtin::storage_power::StoragePowerActorMethods;
using fc::vm::actor::builtin::storage_power::StoragePowerActorState;
using fc::vm::runtime::InvocationOutput;
using fc::vm::runtime::Runtime;
namespace outcome = fc::outcome;

ACTOR_METHOD(StoragePowerActorMethods::construct) {
  if (runtime.getImmediateCaller() != kSystemActorAddress)
    return VMExitCode::STORAGE_POWER_ACTOR_WRONG_CALLER;

  auto datastore = runtime.getIpfsDatastore();
  OUTCOME_TRY(empty_state, StoragePowerActor::createEmptyState(datastore));

  OUTCOME_TRY(runtime.commitState(empty_state));
  return fc::outcome::success();
}

ACTOR_METHOD(StoragePowerActorMethods::addBalance) {
  OUTCOME_TRY(add_balance_params,
              decodeActorParams<AddBalanceParameters>(params));
  OUTCOME_TRY(miner_code_cid, runtime.getActorCodeID(add_balance_params.miner));
  if (miner_code_cid != kStorageMinerCodeCid)
    return VMExitCode::STORAGE_POWER_ILLEGAL_ARGUMENT;

  Address immediate_caller = runtime.getImmediateCaller();
  OUTCOME_TRY(control_addresses,
              requestMinerControlAddress(runtime, add_balance_params.miner));
  if (immediate_caller != control_addresses.owner
      && immediate_caller != control_addresses.worker)
    return VMExitCode::STORAGE_POWER_FORBIDDEN;

  auto datastore = runtime.getIpfsDatastore();
  OUTCOME_TRY(state, datastore->getCbor<StoragePowerActorState>(actor.head));
  StoragePowerActor power_actor(datastore, state);
  OUTCOME_TRY(power_actor.addMinerBalance(add_balance_params.miner,
                                          runtime.getMessage().get().value));

  OUTCOME_TRY(power_actor_state, power_actor.flushState());
  OUTCOME_TRY(runtime.commitState(power_actor_state));
  return fc::outcome::success();
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

  auto datastore = runtime.getIpfsDatastore();
  OUTCOME_TRY(state, datastore->getCbor<StoragePowerActorState>(actor.head));
  StoragePowerActor power_actor(datastore, state);

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
  return fc::outcome::success();
}

fc::outcome::result<InvocationOutput> StoragePowerActorMethods::createMiner(
    const Actor &actor, Runtime &runtime, const MethodParams &params) {
  if (!isSignableActor((actor.code)))
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
  init::ExecParams exec_parameters{kStorageMinerCodeCid,
                                   encoded_construct_miner_parameters};
  OUTCOME_TRY(encoded_exec_parameters, encodeActorParams(exec_parameters));
  OUTCOME_TRY(encoded_addresses_created,
              runtime.send(kInitAddress,
                           init::kExecMethodNumber,
                           encoded_exec_parameters,
                           TokenAmount{0}));
  OUTCOME_TRY(addresses_created,
              decodeActorReturn<init::ExecReturn>(encoded_addresses_created));

  auto datastore = runtime.getIpfsDatastore();
  OUTCOME_TRY(state, datastore->getCbor<StoragePowerActorState>(actor.head));
  StoragePowerActor power_actor(datastore, state);
  OUTCOME_TRY(power_actor.addMiner(addresses_created.id_address));
  OUTCOME_TRY(
      power_actor.setMinerBalance(addresses_created.id_address, message.value));

  CreateMinerReturn result{addresses_created.id_address,
                           addresses_created.robust_address};
  OUTCOME_TRY(output, encodeActorReturn(result));

  OUTCOME_TRY(power_actor_state, power_actor.flushState());
  OUTCOME_TRY(runtime.commitState(power_actor_state));
  return std::move(output);
}

fc::outcome::result<InvocationOutput> StoragePowerActorMethods::deleteMiner(
    const Actor &actor, Runtime &runtime, const MethodParams &params) {
  OUTCOME_TRY(delete_miner_params,
              decodeActorParams<DeleteMinerParameters>(params));

  Address immediate_caller = runtime.getImmediateCaller();
  OUTCOME_TRY(control_addresses,
              requestMinerControlAddress(runtime, delete_miner_params.miner));
  if (immediate_caller != control_addresses.owner
      && immediate_caller != control_addresses.worker)
    return VMExitCode::STORAGE_POWER_FORBIDDEN;

  auto datastore = runtime.getIpfsDatastore();
  OUTCOME_TRY(state, datastore->getCbor<StoragePowerActorState>(actor.head));
  StoragePowerActor power_actor(datastore, state);

  OUTCOME_TRY(power_actor.deleteMiner(delete_miner_params.miner));

  OUTCOME_TRY(power_actor_state, power_actor.flushState());
  OUTCOME_TRY(runtime.commitState(power_actor_state));
  return fc::outcome::success();
}

fc::outcome::result<InvocationOutput>
StoragePowerActorMethods::onSectorProveCommit(const Actor &actor,
                                              Runtime &runtime,
                                              const MethodParams &params) {
  if (actor.code != kStorageMinerCodeCid)
    return VMExitCode::STORAGE_POWER_ACTOR_WRONG_CALLER;

  OUTCOME_TRY(on_sector_prove_commit_params,
              decodeActorParams<OnSectorProveCommitParameters>(params));

  auto datastore = runtime.getIpfsDatastore();
  OUTCOME_TRY(state, datastore->getCbor<StoragePowerActorState>(actor.head));
  StoragePowerActor power_actor(datastore, state);

  StoragePower power =
      consensusPowerForWeight(on_sector_prove_commit_params.weight);
  OUTCOME_TRY(network_power, power_actor.getTotalNetworkPower());
  TokenAmount pledge =
      pledgeForWeight(on_sector_prove_commit_params.weight, network_power);
  OUTCOME_TRY(
      power_actor.addToClaim(runtime.getMessage().get().from, power, pledge));

  OnSectorProveCommitReturn exec_return{pledge};
  OUTCOME_TRY(result, encodeActorReturn(exec_return));

  OUTCOME_TRY(power_actor_state, power_actor.flushState());
  OUTCOME_TRY(runtime.commitState(power_actor_state));
  return std::move(result);
}

fc::outcome::result<InvocationOutput>
StoragePowerActorMethods::onSectorTerminate(const Actor &actor,
                                            Runtime &runtime,
                                            const MethodParams &params) {
  if (actor.code != kStorageMinerCodeCid)
    return VMExitCode::STORAGE_POWER_ACTOR_WRONG_CALLER;

  OUTCOME_TRY(on_sector_terminate_params,
              decodeActorParams<OnSectorTerminateParameters>(params));

  Address miner_address = runtime.getMessage().get().from;
  StoragePower power =
      consensusPowerForWeights(on_sector_terminate_params.weights);

  auto datastore = runtime.getIpfsDatastore();
  OUTCOME_TRY(state, datastore->getCbor<StoragePowerActorState>(actor.head));
  StoragePowerActor power_actor(datastore, state);

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
  return fc::outcome::success();
}

fc::outcome::result<InvocationOutput>
StoragePowerActorMethods::onSectorTemporaryFaultEffectiveBegin(
    const Actor &actor, Runtime &runtime, const MethodParams &params) {
  if (actor.code != kStorageMinerCodeCid)
    return VMExitCode::STORAGE_POWER_ACTOR_WRONG_CALLER;

  OUTCOME_TRY(on_sector_fault_params,
              decodeActorParams<OnSectorTemporaryFaultEffectiveBeginParameters>(
                  params));
  StoragePower power = consensusPowerForWeights(on_sector_fault_params.weights);

  auto datastore = runtime.getIpfsDatastore();
  OUTCOME_TRY(state, datastore->getCbor<StoragePowerActorState>(actor.head));
  StoragePowerActor power_actor(datastore, state);

  OUTCOME_TRY(power_actor.addToClaim(
      runtime.getMessage().get().from, -power, -on_sector_fault_params.pledge));

  OUTCOME_TRY(power_actor_state, power_actor.flushState());
  OUTCOME_TRY(runtime.commitState(power_actor_state));
  return fc::outcome::success();
}

fc::outcome::result<InvocationOutput>
StoragePowerActorMethods::slashPledgeCollateral(Runtime &runtime,
                                                StoragePowerActor &power_actor,
                                                Address miner,
                                                TokenAmount to_slash) {
  OUTCOME_TRY(
      slashed,
      power_actor.subtractMinerBalance(miner, to_slash, TokenAmount{0}));
  OUTCOME_TRY(
      runtime.send(kBurntFundsActorAddress, kSendMethodNumber, {}, slashed));

  return fc::outcome::success();
}

const ActorExports fc::vm::actor::builtin::storage_power::exports = {
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
         StoragePowerActorMethods::onSectorTemporaryFaultEffectiveBegin)}};
