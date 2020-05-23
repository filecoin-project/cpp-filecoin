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
  using actor::kAccountCodeCid;
  using actor::kEmptyObjectCid;
  using actor::kRewardAddress;
  using actor::kSendMethodNumber;
  using actor::kSystemActorAddress;
  using storage::hamt::HamtError;

  outcome::result<MessageReceipt> Env::applyMessage(
      const UnsignedMessage &message, TokenAmount &penalty) {
    if (message.gasLimit <= 0) {
      return RuntimeError::UNKNOWN;
    }

    auto execution = Execution::make(shared_from_this(), message);
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
    OUTCOME_TRY(execution->chargeGas(msg_gas_cost));
    auto result = execution->send(message);
    auto exit_code = VMExitCode::Ok;
    if (!result) {
      if (!isVMExitCode(result.error())) {
        return result.error();
      }
      exit_code = VMExitCode{result.error().value()};
    } else {
      receipt.return_value = std::move(result.value());
      auto result_charged = execution->chargeGas(
          receipt.return_value.size() * kOnChainReturnValuePerByteCost);
      if (!result_charged) {
        BOOST_ASSERT(isVMExitCode(result_charged.error()));
        exit_code = VMExitCode{result_charged.error().value()};
      }
    }
    if (exit_code != VMExitCode::Ok) {
      OUTCOME_TRY(state_tree->revert(snapshot));
    }

    BOOST_ASSERT_MSG(execution->gas_used >= 0, "negative used gas");
    BOOST_ASSERT_MSG(execution->gas_used <= message.gasLimit,
                     "runtime charged gas over limit");

    auto gas_refund = message.gasLimit - execution->gas_used;
    if (gas_refund != 0) {
      OUTCOME_TRY(from, state_tree->get(message.from));
      from.balance += gas_refund * message.gasPrice;
      OUTCOME_TRY(state_tree->set(message.from, from));
    }

    OUTCOME_TRY(reward, state_tree->get(kRewardAddress));
    reward.balance += execution->gas_used * message.gasPrice;
    OUTCOME_TRY(state_tree->set(kRewardAddress, reward));

    auto ret_code = normalizeVMExitCode(exit_code);
    BOOST_ASSERT_MSG(ret_code, "c++ actor code returned unknown error");
    penalty = 0;
    receipt.exit_code = *ret_code;
    receipt.gas_used = execution->gas_used;
    return receipt;
  }

  outcome::result<InvocationOutput> Env::applyImplicitMessage(
      UnsignedMessage message) {
    OUTCOME_TRY(from, state_tree->get(message.from));
    message.nonce = from.nonce;
    auto execution = Execution::make(shared_from_this(), message);
    return execution->send(message);
  }

  outcome::result<void> Execution::chargeGas(GasAmount amount) {
    gas_used += amount;
    if (gas_limit != kInfiniteGas && gas_used > gas_limit) {
      gas_used = gas_limit;
      return VMExitCode::SysErrOutOfGas;
    }
    return outcome::success();
  }

  std::shared_ptr<Execution> Execution::make(std::shared_ptr<Env> env,
                                             const UnsignedMessage &message) {
    auto execution = std::make_shared<Execution>();
    execution->env = env;
    execution->state_tree = env->state_tree;
    execution->gas_used = 0;
    execution->gas_limit = message.gasLimit;
    execution->origin = message.from;
    return execution;
  }

  outcome::result<Actor> Execution::tryCreateAccountActor(
      const Address &address) {
    if (!address.isKeyType()) {
      return VMExitCode{1};
    }
    OUTCOME_TRY(id, state_tree->registerNewAddress(address));
    OUTCOME_TRY(chargeGas(kCreateActorGasCost));
    OUTCOME_TRY(state_tree->set(id,
                                {actor::kAccountCodeCid,
                                 ActorSubstateCID{actor::kEmptyObjectCid},
                                 {},
                                 {}}));
    OUTCOME_TRY(params, actor::encodeActorParams(address));
    OUTCOME_TRY(sendWithRevert({0,
                                id,
                                kSystemActorAddress,
                                {},
                                {},
                                {},
                                {},
                                actor::builtin::account::Construct::Number,
                                params}));
    return state_tree->get(id);
  }

  outcome::result<InvocationOutput> Execution::sendWithRevert(
      const UnsignedMessage &message) {
    OUTCOME_TRY(snapshot, state_tree->flush());
    auto result = send(message);
    if (!result) {
      OUTCOME_TRY(state_tree->revert(snapshot));
      return result.error();
    }
    return result;
  }

  outcome::result<InvocationOutput> Execution::send(
      const UnsignedMessage &message) {
    if (message.value != 0) {
      OUTCOME_TRY(chargeGas(kSendTransferFundsGasCost));
    }
    if (message.method != kSendMethodNumber) {
      OUTCOME_TRY(chargeGas(kSendInvokeMethodGasCost));
    }

    Actor to_actor;
    auto maybe_to_actor = state_tree->get(message.to);
    if (!maybe_to_actor) {
      if (maybe_to_actor.error() != HamtError::NOT_FOUND) {
        return maybe_to_actor.error();
      }
      OUTCOME_TRY(account_actor, tryCreateAccountActor(message.to));
      to_actor = account_actor;
    } else {
      to_actor = maybe_to_actor.value();
    }
    RuntimeImpl runtime{shared_from_this(), message, to_actor.head};

    if (message.value != 0) {
      BOOST_ASSERT(message.value > 0);
      OUTCOME_TRY(from_actor, state_tree->get(message.from));
      if (from_actor.balance < message.value) {
        return VMExitCode::SEND_TRANSFER_INSUFFICIENT;
      }
      from_actor.balance -= message.value;
      to_actor.balance += message.value;
      OUTCOME_TRY(state_tree->set(message.to, to_actor));
      OUTCOME_TRY(state_tree->set(message.from, from_actor));
    }

    if (message.method != kSendMethodNumber) {
      auto result = env->invoker->invoke(
          to_actor, runtime, message.method, message.params);
      OUTCOME_TRYA(to_actor, state_tree->get(message.to));
      to_actor.head = runtime.getCurrentActorState();
      OUTCOME_TRY(state_tree->set(message.to, to_actor));
      return result;
    }

    return outcome::success();
  }
}  // namespace fc::vm::runtime
