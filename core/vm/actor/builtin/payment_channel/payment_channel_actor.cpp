/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/payment_channel/payment_channel_actor.hpp"

#include "vm/actor/actor_method.hpp"
#include "vm/exit_code/exit_code.hpp"

using fc::outcome::result;
using fc::primitives::address::Protocol;
using fc::vm::actor::decodeActorParams;
using fc::vm::actor::kAccountCodeCid;
using fc::vm::actor::builtin::payment_channel::PaymentChannelActor;
using fc::vm::runtime::InvocationOutput;
using fc::vm::runtime::Runtime;

fc::outcome::result<InvocationOutput> PaymentChannelActor::construct(
    const Actor &actor, Runtime &runtime, const MethodParams &params) {
  if (actor.code != kAccountCodeCid)
    return VMExitCode::PAYMENT_CHANNEL_WRONG_CALLER;

  OUTCOME_TRY(construct_params,
              decodeActorParams<ConstructParameteres>(params));
  if (construct_params.to.getProtocol() != Protocol::ID)
    return VMExitCode::PAYMENT_CHANNEL_ILLEGAL_ARGUMENT;
  OUTCOME_TRY(target_actor_code_id,
              runtime.getActorCodeID(construct_params.to));
  if (target_actor_code_id != kAccountCodeCid)
    return VMExitCode::PAYMENT_CHANNEL_ILLEGAL_ARGUMENT;

  PaymentChannelActorState state{
      runtime.getImmediateCaller(), construct_params.to, 0, 0, 0, {}};

  OUTCOME_TRY(runtime.commitState(state));
  return fc::outcome::success();
}

fc::outcome::result<InvocationOutput> PaymentChannelActor::updateChannelState(
    const Actor &actor, Runtime &runtime, const MethodParams &params) {
  OUTCOME_TRY(state,
              runtime.getIpfsDatastore()->getCbor<PaymentChannelActorState>(
                  actor.head));
  Address signer = runtime.getImmediateCaller();
  if (signer != state.from || signer != state.to) {
    return VMExitCode::PAYMENT_CHANNEL_WRONG_CALLER;
  }

  OUTCOME_TRY(update_state_params,
              decodeActorParams<UpdateChannelStateParameters>(params));

  // TODO (a.chernyshov) not implemented yet FIL-129

  return fc::outcome::success();
}

fc::outcome::result<InvocationOutput> PaymentChannelActor::settle(
    const Actor &actor, Runtime &runtime, const MethodParams &params) {

  // TODO (a.chernyshov) not implemented yet FIL-129

  return fc::outcome::success();
}

fc::outcome::result<InvocationOutput> PaymentChannelActor::collect(
    const Actor &actor, Runtime &runtime, const MethodParams &params) {

  // TODO (a.chernyshov) not implemented yet FIL-129

  return fc::outcome::success();
}
