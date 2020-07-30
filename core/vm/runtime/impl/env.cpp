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
      return RuntimeError::kUnknown;
    }

    auto execution = Execution::make(shared_from_this(), message);
    MessageReceipt receipt;
    receipt.gas_used = 0;

    OUTCOME_TRY(serialized_message, codec::cbor::encode(message));
    auto msg_gas_cost{pricelist.onChainMessage(serialized_message.size())};
    penalty = msg_gas_cost * message.gasPrice;
    if (msg_gas_cost > message.gasLimit) {
      receipt.exit_code = VMExitCode::kSysErrOutOfGas;
      return receipt;
    }

    auto maybe_from = state_tree->get(message.from);
    if (!maybe_from) {
      if (maybe_from.error() == HamtError::kNotFound) {
        receipt.exit_code = VMExitCode::kSysErrSenderInvalid;
        return receipt;
      }
      return maybe_from.error();
    }
    auto &from = maybe_from.value();
    if (from.code != actor::kAccountCodeCid) {
      receipt.exit_code = VMExitCode::kSysErrSenderInvalid;
      return receipt;
    }
    if (message.nonce != from.nonce) {
      receipt.exit_code = VMExitCode::kSysErrSenderStateInvalid;
      return receipt;
    }

    BigInt gas_cost = message.gasLimit * message.gasPrice;
    if (from.balance < gas_cost + message.value) {
      receipt.exit_code = VMExitCode::kSysErrSenderStateInvalid;
      return receipt;
    }
    from.balance -= gas_cost;
    ++from.nonce;
    OUTCOME_TRY(state_tree->set(message.from, from));

    OUTCOME_TRY(snapshot, state_tree->flush());
    OUTCOME_TRY(execution->chargeGas(msg_gas_cost));
    auto result = execution->send(message);
    auto exit_code = VMExitCode::kOk;
    if (!result) {
      if (!isVMExitCode(result.error())) {
        return result.error();
      }
      exit_code = VMExitCode{result.error().value()};
    } else {
      receipt.return_value = std::move(result.value());
      auto result_charged = execution->chargeGas(
          pricelist.onChainReturnValue(receipt.return_value.size()));
      if (!result_charged) {
        BOOST_ASSERT(isVMExitCode(result_charged.error()));
        exit_code = VMExitCode{result_charged.error().value()};
        receipt.return_value.clear();
      }
    }
    if (exit_code != VMExitCode::kOk) {
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
      return VMExitCode::kSysErrOutOfGas;
    }
    return outcome::success();
  }

  std::shared_ptr<Execution> Execution::make(std::shared_ptr<Env> env,
                                             const UnsignedMessage &message) {
    auto execution = std::make_shared<Execution>();
    execution->env = env;
    execution->state_tree = env->state_tree;
    execution->charging_ipld = std::make_shared<ChargingIpld>(execution);
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
    OUTCOME_TRY(state_tree->set(
        id, {actor::kAccountCodeCid, actor::kEmptyObjectCid, {}, {}}));
    OUTCOME_TRY(params, actor::encodeActorParams(address));
    OUTCOME_TRY(sendWithRevert({id,
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
    OUTCOME_TRY(chargeGas(env->pricelist.onMethodInvocation(
        message.value, message.method.method_number)));

    Actor to_actor;
    auto maybe_to_actor = state_tree->get(message.to);
    if (!maybe_to_actor) {
      if (maybe_to_actor.error() != HamtError::kNotFound) {
        return maybe_to_actor.error();
      }
      OUTCOME_TRY(account_actor, tryCreateAccountActor(message.to));
      to_actor = account_actor;
    } else {
      to_actor = maybe_to_actor.value();
    }
    OUTCOME_TRY(caller_id, state_tree->lookupId(message.from));
    RuntimeImpl runtime{shared_from_this(), message, caller_id, to_actor.head};

    if (message.value != 0) {
      BOOST_ASSERT(message.value > 0);
      OUTCOME_TRY(from_actor, state_tree->get(message.from));
      if (from_actor.balance < message.value) {
        return VMExitCode::kSendTransferInsufficient;
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

  // lotus read-write patterns differ, causing different gas in receipts
  constexpr auto kDisableChargingIpld{true};

  outcome::result<void> ChargingIpld::set(const CID &key, Value value) {
    auto execution{execution_.lock()};
    if (!kDisableChargingIpld) {
      OUTCOME_TRY(execution->chargeGas(
          execution->env->pricelist.onIpldPut(value.size())));
    }
    return execution->env->ipld->set(key, value);
  }

  outcome::result<Ipld::Value> ChargingIpld::get(const CID &key) const {
    auto execution{execution_.lock()};
    OUTCOME_TRY(value, execution->env->ipld->get(key));
    if (!kDisableChargingIpld) {
      OUTCOME_TRY(execution->chargeGas(
          execution->env->pricelist.onIpldGet(value.size())));
    }
    return std::move(value);
  }
}  // namespace fc::vm::runtime
