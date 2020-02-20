/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/storage_power/storage_power_actor_export.hpp"

#include "vm/actor/builtin/init/init_actor.hpp"
#include "vm/actor/builtin/miner/miner_actor.hpp"
#include "vm/actor/builtin/shared/shared.hpp"

using fc::primitives::address::decode;
using fc::vm::actor::ActorExports;
using fc::vm::actor::kConstructorMethodNumber;
using fc::vm::actor::builtin::requestMinerControlAddress;
using fc::vm::actor::builtin::storage_power::StoragePowerActorMethods;
using fc::vm::actor::builtin::storage_power::StoragePowerActorState;
using fc::vm::runtime::InvocationOutput;
using fc::vm::runtime::Runtime;

fc::outcome::result<InvocationOutput> StoragePowerActorMethods::construct(
    const Actor &actor, Runtime &runtime, const MethodParams &params) {
  if (runtime.getImmediateCaller() != kSystemActorAddress)
    return VMExitCode::STORAGE_POWER_ACTOR_WRONG_CALLER;

  auto datastore = runtime.getIpfsDatastore();
  OUTCOME_TRY(empty_state, StoragePowerActor::createEmptyState(datastore));

  // commit state
  OUTCOME_TRY(state_cid, datastore->setCbor(empty_state));
  OUTCOME_TRY(runtime.commit(ActorSubstateCID{state_cid}));

  return fc::outcome::success();
}

fc::outcome::result<InvocationOutput> StoragePowerActorMethods::addBalance(
    const Actor &actor, Runtime &runtime, const MethodParams &params) {
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

  // commit state
  OUTCOME_TRY(power_actor_state, power_actor.flushState());
  OUTCOME_TRY(state_cid, datastore->setCbor(power_actor_state));
  OUTCOME_TRY(runtime.commit(ActorSubstateCID{state_cid}));

  return fc::outcome::success();
}

fc::outcome::result<InvocationOutput> StoragePowerActorMethods::withdrawBalance(
    const Actor &actor, Runtime &runtime, const MethodParams &params) {
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
   * Pledge for sectors in temporary fault has already been subtracted from the
   * claim. If the miner has failed a scheduled PoSt, collateral remains locked
   * for further penalization. Thus the current claimed pledge is the amount to
   * keep locked.
   */
  OUTCOME_TRY(
      subtracted,
      power_actor.subtractMinerBalance(withdraw_balance_params.miner,
                                       withdraw_balance_params.requested,
                                       claim.pledge));

  OUTCOME_TRY(
      runtime.send(control_addresses.owner, kSendMethodNumber, {}, subtracted));

  // commit state
  OUTCOME_TRY(power_actor_state, power_actor.flushState());
  OUTCOME_TRY(state_cid, datastore->setCbor(power_actor_state));
  OUTCOME_TRY(runtime.commit(ActorSubstateCID{state_cid}));

  return fc::outcome::success();
}

fc::outcome::result<InvocationOutput> StoragePowerActorMethods::createMiner(
    const Actor &actor, Runtime &runtime, const MethodParams &params) {
  if (!isSignableActor((actor.code)))
    return VMExitCode::STORAGE_POWER_FORBIDDEN;

  OUTCOME_TRY(create_miner_params,
              decodeActorParams<CreateMinerParameters>(params));

  auto message = runtime.getMessage().get();
  miner::ConstructParameters construct_miner_parameters{
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

  // commit state
  OUTCOME_TRY(power_actor_state, power_actor.flushState());
  OUTCOME_TRY(state_cid, datastore->setCbor(power_actor_state));
  OUTCOME_TRY(runtime.commit(ActorSubstateCID{state_cid}));

  return std::move(output);
}

const ActorExports fc::vm::actor::builtin::storage_power::exports = {
    {kConstructorMethodNumber,
     ActorMethod(StoragePowerActorMethods::construct)},
    {kAddBalanceMethodNumber,
     ActorMethod(StoragePowerActorMethods::addBalance)},
    {kWithdrawBalanceMethodNumber,
     ActorMethod(StoragePowerActorMethods::withdrawBalance)},
    {kCreateMinerMethodNumber,
     ActorMethod(StoragePowerActorMethods::createMiner)}};
