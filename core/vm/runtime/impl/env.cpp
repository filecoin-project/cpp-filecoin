/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/env.hpp"

#include "vm/actor/builtin/account/account_actor.hpp"
#include "vm/actor/cgo/actors.hpp"
#include "vm/exit_code/exit_code.hpp"
#include "vm/runtime/impl/runtime_impl.hpp"
#include "vm/runtime/runtime_error.hpp"

namespace fc::vm::runtime {
  using actor::kAccountCodeCid;
  using actor::kEmptyObjectCid;
  using actor::kRewardAddress;
  using actor::kSendMethodNumber;
  using actor::kSystemActorAddress;
  using storage::hamt::HamtError;

  outcome::result<Address> resolveKey(StateTree &state_tree,
                                      const Address &address) {
    if (address.isKeyType()) {
      return address;
    }
    if (auto _actor{state_tree.get(address)}) {
      auto &actor{_actor.value()};
      if (actor.code == kAccountCodeCid) {
        if (auto _state{
                state_tree.getStore()
                    ->getCbor<actor::builtin::account::AccountActorState>(
                        actor.head)}) {
          return _state.value().address;
        }
      }
    }
    return VMExitCode::kSysErrInvalidParameters;
  }

  outcome::result<Env::Apply> Env::applyMessage(const UnsignedMessage &message,
                                                size_t size) {
    TokenAmount locked;
    auto add_locked{
        [&](auto &address, const TokenAmount &add) -> outcome::result<void> {
          if (add != 0) {
            OUTCOME_TRY(actor, state_tree->get(address));
            actor.balance += add;
            locked -= add;
            OUTCOME_TRY(state_tree->set(address, actor));
          }
          return outcome::success();
        }};
    if (message.gas_limit <= 0) {
      return RuntimeError::kUnknown;
    }
    auto execution = Execution::make(shared_from_this(), message);
    Apply apply;
    auto msg_gas_cost{pricelist.onChainMessage(size)};
    if (msg_gas_cost > message.gas_limit) {
      apply.penalty = msg_gas_cost * tipset.getParentBaseFee();
      apply.receipt.exit_code = VMExitCode::kSysErrOutOfGas;
      return apply;
    }
    apply.penalty = message.gas_limit * tipset.getParentBaseFee();
    auto maybe_from = state_tree->get(message.from);
    if (!maybe_from) {
      if (maybe_from.error() == HamtError::kNotFound) {
        apply.receipt.exit_code = VMExitCode::kSysErrSenderInvalid;
        return apply;
      }
      return maybe_from.error();
    }
    auto &from = maybe_from.value();
    if (from.code != actor::kAccountCodeCid) {
      apply.receipt.exit_code = VMExitCode::kSysErrSenderInvalid;
      return apply;
    }
    if (message.nonce != from.nonce) {
      apply.receipt.exit_code = VMExitCode::kSysErrSenderStateInvalid;
      return apply;
    }
    BigInt gas_cost{message.gas_limit * message.gas_fee_cap};
    if (from.balance < gas_cost) {
      apply.receipt.exit_code = VMExitCode::kSysErrSenderStateInvalid;
      return apply;
    }
    OUTCOME_TRY(add_locked(message.from, -gas_cost));
    OUTCOME_TRYA(from, state_tree->get(message.from));
    ++from.nonce;
    OUTCOME_TRY(state_tree->set(message.from, from));

    OUTCOME_TRY(snapshot, state_tree->flush());
    auto result{execution->send(message, msg_gas_cost)};
    auto exit_code = VMExitCode::kOk;
    if (!result) {
      if (!isVMExitCode(result.error())) {
        return result.error();
      }
      exit_code = VMExitCode{result.error().value()};
      if (exit_code == VMExitCode::kFatal) {
        return result.error();
      }
    } else {
      auto &ret{result.value()};
      if (!ret.empty()) {
        auto charge =
            execution->chargeGas(pricelist.onChainReturnValue(ret.size()));
        if (!charge) {
          BOOST_ASSERT(isVMExitCode(charge.error()));
          exit_code = VMExitCode{charge.error().value()};
        } else {
          apply.receipt.return_value = std::move(ret);
        }
      }
    }
    if (exit_code != VMExitCode::kOk) {
      OUTCOME_TRY(state_tree->revert(snapshot));
    }
    auto limit{message.gas_limit}, &used{execution->gas_used};
    if (used < 0) {
      used = 0;
    }
    BOOST_ASSERT_MSG(used <= limit, "runtime charged gas over limit");
    auto base_fee{tipset.getParentBaseFee()}, fee_cap{message.gas_fee_cap},
        base_fee_pay{std::min(base_fee, fee_cap)};
    apply.penalty = base_fee > fee_cap ? TokenAmount{base_fee - fee_cap} * used
                                       : TokenAmount{0};
    OUTCOME_TRY(
        add_locked(actor::kBurntFundsActorAddress, base_fee_pay * used));
    apply.reward =
        std::min(message.gas_premium, TokenAmount{fee_cap - base_fee_pay})
        * limit;
    OUTCOME_TRY(add_locked(kRewardAddress, apply.reward));
    auto over{limit - 11 * used / 10};
    auto gas_burned{
        used == 0
            ? limit
            : over < 0 ? 0
                       : (GasAmount)bigdiv(
                           BigInt{limit - used} * std::min(used, over), used)};
    if (gas_burned != 0) {
      OUTCOME_TRY(add_locked(actor::kBurntFundsActorAddress,
                             base_fee_pay * gas_burned));
      apply.penalty += (base_fee - base_fee_pay) * gas_burned;
    }
    BOOST_ASSERT_MSG(locked >= 0, "gas math wrong");
    OUTCOME_TRY(add_locked(message.from, locked));
    apply.receipt.exit_code = exit_code;
    apply.receipt.gas_used = used;
    return apply;
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
    if (gas_used > gas_limit) {
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
    execution->gas_limit = message.gas_limit;
    execution->origin = message.from;
    execution->origin_nonce = message.nonce;
    return execution;
  }

  outcome::result<Actor> Execution::tryCreateAccountActor(
      const Address &address) {
    OUTCOME_TRY(chargeGas(env->pricelist.onCreateActor()));
    OUTCOME_TRY(id, state_tree->registerNewAddress(address));
    if (!address.isKeyType()) {
      return VMExitCode::kSysErrInvalidReceiver;
    }
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
      const UnsignedMessage &message, GasAmount charge) {
    OUTCOME_TRY(chargeGas(charge));
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
    OUTCOME_TRY(chargeGas(env->pricelist.onMethodInvocation(
        message.value, message.method.method_number)));
    OUTCOME_TRY(caller_id, state_tree->lookupId(message.from));
    RuntimeImpl runtime{shared_from_this(), message, caller_id};

    if (message.value != 0) {
      if (message.value < 0) {
        return VMExitCode::kSysErrForbidden;
      }
      OUTCOME_TRY(to_id, state_tree->lookupId(message.to));
      if (to_id != caller_id) {
        OUTCOME_TRY(from_actor, state_tree->get(caller_id));
        if (from_actor.balance < message.value) {
          return VMExitCode::kSysErrInsufficientFunds;
        }
        from_actor.balance -= message.value;
        to_actor.balance += message.value;
        OUTCOME_TRY(state_tree->set(caller_id, from_actor));
        OUTCOME_TRY(state_tree->set(to_id, to_actor));
      }
    }

    if (message.method != kSendMethodNumber) {
      auto _message{message};
      _message.from = caller_id;
      // TODO: check cpp actor
      return actor::cgo::invoke(shared_from_this(),
                                _message,
                                to_actor.code,
                                message.method.method_number,
                                message.params);
    }

    return outcome::success();
  }

  outcome::result<void> ChargingIpld::set(const CID &key, Value value) {
    auto execution{execution_.lock()};
    OUTCOME_TRY(execution->chargeGas(
        execution->env->pricelist.onIpldPut(value.size())));
    return execution->env->ipld->set(key, std::move(value));
  }

  outcome::result<Ipld::Value> ChargingIpld::get(const CID &key) const {
    auto execution{execution_.lock()};
    OUTCOME_TRY(value, execution->env->ipld->get(key));
    OUTCOME_TRY(execution->chargeGas(execution->env->pricelist.onIpldGet()));
    return std::move(value);
  }
}  // namespace fc::vm::runtime
