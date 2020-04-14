/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/env.hpp"

#include "vm/actor/builtin/account/account_actor.hpp"
#include "vm/exit_code/exit_code.hpp"
#include "vm/runtime/gas_cost.hpp"
#include "vm/runtime/impl/runtime_impl.hpp"
#include "vm/runtime/runtime_error.hpp"

namespace fc::vm::runtime {
  using actor::kRewardAddress;
  using actor::kSendMethodNumber;
  using actor::builtin::account::AccountActor;
  using storage::hamt::HamtError;

  outcome::result<MessageReceipt> Env::applyMessage(
      const UnsignedMessage &message, TokenAmount &penalty) {
    if (message.gasLimit <= 0) {
      return RuntimeError::UNKNOWN;
    }

    MessageReceipt receipt;
    receipt.gas_used = 0;

    OUTCOME_TRY(serialized_message, codec::cbor::encode(message));
    GasAmount msg_gas_cost =
        kOnChainMessageBaseGasCost
        + serialized_message.size() * kOnChainMessagePerByteGasCharge;
    penalty = msg_gas_cost * message.gasPrice;
    if (msg_gas_cost > message.gasLimit) {
      receipt.exit_code = VMExitCode::SysErrOutOfGas;
      return receipt;
    }

    auto maybe_from = state_tree->get(message.from);
    if (!maybe_from) {
      if (maybe_from.error() == HamtError::NOT_FOUND) {
        receipt.exit_code = VMExitCode::SysErrSenderInvalid;
        return receipt;
      }
      return maybe_from.error();
    }
    auto &from = maybe_from.value();
    if (from.code != actor::kAccountCodeCid) {
      receipt.exit_code = VMExitCode::SysErrSenderInvalid;
      return receipt;
    }
    if (message.nonce != from.nonce) {
      receipt.exit_code = VMExitCode::SysErrSenderStateInvalid;
      return receipt;
    }

    BigInt gas_cost = message.gasLimit * message.gasPrice;
    if (from.balance < gas_cost + message.value) {
      receipt.exit_code = VMExitCode::SysErrSenderStateInvalid;
      return receipt;
    }
    from.balance -= gas_cost;
    ++from.nonce;
    OUTCOME_TRY(state_tree->set(message.from, from));

    OUTCOME_TRY(snapshot, state_tree->flush());
    receipt.gas_used = msg_gas_cost;
    auto result = send(receipt.gas_used, message.from, message);
    auto exit_code = VMExitCode::Ok;
    if (!result) {
      if (!isVMExitCode(result.error())) {
        return result.error();
      }
      exit_code = VMExitCode{result.error().value()};
    } else {
      receipt.return_value = std::move(result.value());
      auto result_charged = RuntimeImpl::chargeGas(
          receipt.gas_used,
          message.gasLimit,
          receipt.return_value.size() * kOnChainReturnValuePerByteCost);
      if (!result_charged) {
        BOOST_ASSERT(isVMExitCode(result_charged.error()));
        exit_code = VMExitCode{result_charged.error().value()};
      }
    }
    if (exit_code != VMExitCode::Ok) {
      OUTCOME_TRY(state_tree->revert(snapshot));
    }

    BOOST_ASSERT_MSG(receipt.gas_used >= 0, "negative used gas");
    BOOST_ASSERT_MSG(receipt.gas_used <= message.gasLimit,
                     "runtime charged gas over limit");

    auto gas_refund = message.gasLimit - receipt.gas_used;
    if (gas_refund != 0) {
      OUTCOME_TRY(from, state_tree->get(message.from));
      from.balance += gas_refund * message.gasPrice;
      OUTCOME_TRY(state_tree->set(message.from, from));
    }

    OUTCOME_TRY(reward, state_tree->get(kRewardAddress));
    reward.balance += receipt.gas_used * message.gasPrice;
    OUTCOME_TRY(state_tree->set(kRewardAddress, reward));

    auto ret_code = normalizeVMExitCode(exit_code);
    BOOST_ASSERT_MSG(ret_code, "c++ actor code returned unknown error");
    penalty = 0;
    receipt.exit_code = *ret_code;
    return receipt;
  }

  outcome::result<InvocationOutput> Env::send(GasAmount &gas_used,
                                              const Address &origin,
                                              const UnsignedMessage &message) {
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
    RuntimeImpl runtime{shared_from_this(),
                        message,
                        origin,
                        message.gasLimit,
                        gas_used,
                        to_actor.head};

    if (message.value) {
      OUTCOME_TRY(runtime.chargeGas(kSendTransferFundsGasCost));
      OUTCOME_TRY(from_actor, state_tree->get(message.from));
      OUTCOME_TRY(RuntimeImpl::transfer(from_actor, to_actor, message.value));
      OUTCOME_TRY(state_tree->set(message.to, to_actor));
      OUTCOME_TRY(state_tree->set(message.from, from_actor));
    }

    InvocationOutput invocation_output;
    if (message.method != kSendMethodNumber) {
      OUTCOME_TRY(runtime.chargeGas(kSendInvokeMethodGasCost));
      auto invocation =
          invoker->invoke(to_actor, runtime, message.method, message.params);
      if (invocation || isVMExitCode(invocation.error())) {
        OUTCOME_TRY(to_actor_2, state_tree->get(message.to));
        to_actor_2.head = runtime.getCurrentActorState();
        OUTCOME_TRY(state_tree->set(message.to, to_actor_2));
      }
      invocation_output = invocation.value();
    }

    gas_used = runtime.gasUsed();

    return invocation_output;
  }
}  // namespace fc::vm::runtime
