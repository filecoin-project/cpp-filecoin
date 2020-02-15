/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/env.hpp"

#include "vm/actor/builtin/account/account_actor.hpp"
#include "vm/exit_code/exit_code.hpp"
#include "vm/message/message_codec.hpp"
#include "vm/runtime/gas_cost.hpp"
#include "vm/runtime/impl/runtime_impl.hpp"
#include "vm/runtime/runtime_error.hpp"

namespace fc::vm::runtime {
  using actor::builtin::account::AccountActor;
  using storage::hamt::HamtError;

  outcome::result<MessageReceipt> Env::applyMessage(const UnsignedMessage &message) {
    BigInt gas_cost = message.gasLimit * message.gasPrice;
    BigInt total_cost = gas_cost + message.value;

    Actor gas_holder;
    gas_holder.balance = 0;

    OUTCOME_TRY(from_actor, state_tree->get(message.from));
    if (from_actor.balance < total_cost) {
      return RuntimeError::NOT_ENOUGH_FUNDS;
    }
    OUTCOME_TRY(RuntimeImpl::transfer(from_actor, gas_holder, gas_cost));
    // TODO: check actor nonce matches message nonce
    ++from_actor.nonce;
    OUTCOME_TRY(state_tree->set(message.from, from_actor));

    OUTCOME_TRY(serialized_message, codec::cbor::encode(message));
    BigInt gas_used = kOnChainMessageBaseGasCost + serialized_message.size() * kOnChainMessagePerByteGasCharge;

    auto result = send(gas_used, message.from, message);
    if (!result)  {
      if (!isVMExitCode(result.error())) {
        return result.error();
      }
      gas_used = message.gasLimit;
    } else {
      OUTCOME_TRY(from_actor_2, state_tree->get(message.from));
      OUTCOME_TRY(RuntimeImpl::transfer(gas_holder, from_actor_2, (message.gasLimit - gas_used) * message.gasPrice));
      OUTCOME_TRY(state_tree->set(message.from, from_actor_2));
    }

    OUTCOME_TRY(miner_actor, state_tree->get(block_miner));
    OUTCOME_TRY(RuntimeImpl::transfer(gas_holder, miner_actor, gas_used * message.gasPrice));
    OUTCOME_TRY(state_tree->set(block_miner, miner_actor));

    OUTCOME_TRY(ret_code, getRetCode(result));
    return MessageReceipt{
      ret_code,
      result ? result.value().return_value : Buffer{},
      gas_used,
    };
  }

  outcome::result<InvocationOutput> Env::send(BigInt &gas_used, const Address &origin, const UnsignedMessage &message) {
    Actor to_actor;
    auto maybe_to_actor = state_tree->get(message.to);
    if (!maybe_to_actor) {
      if (maybe_to_actor.error() != HamtError::NOT_FOUND) {
        return maybe_to_actor.error();
      }
      OUTCOME_TRY(account_actor, AccountActor::create(state_tree, message.to));
      to_actor = account_actor;
    } else {
      to_actor = maybe_to_actor.value();
    }
    RuntimeImpl runtime{shared_from_this(), message, origin, message.gasLimit, gas_used, to_actor.head};

    if (message.value) {
      OUTCOME_TRY(runtime.chargeGas(kSendTransferFundsGasCost));
      OUTCOME_TRY(from_actor, state_tree->get(message.from));
      OUTCOME_TRY(RuntimeImpl::transfer(from_actor, to_actor, message.value));
      OUTCOME_TRY(state_tree->set(message.to, to_actor));
      OUTCOME_TRY(state_tree->set(message.from, from_actor));
    }

    InvocationOutput invocation_output;
    if (message.method.method_number) {
      OUTCOME_TRY(runtime.chargeGas(kSendInvokeMethodGasCost));
      auto invocation = invoker->invoke(to_actor, runtime, message.method, message.params);
      if (invocation || isVMExitCode(invocation.error())) {
        OUTCOME_TRY(to_actor_2, state_tree->get(message.to));
        to_actor_2.head = runtime.getCurrentActorState();
        OUTCOME_TRY(state_tree->set(message.to, to_actor_2));
      }
      invocation_output = invocation.value();
    }

    gas_used += runtime.gasUsed();

    return invocation_output;
  }
}  // namespace fc::vm::runtime
