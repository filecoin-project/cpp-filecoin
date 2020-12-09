/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/env.hpp"

#include "vm/actor/builtin/v0/account/account_actor.hpp"
#include "vm/actor/builtin/v0/codes.hpp"
#include "vm/actor/builtin/v2/account/account_actor.hpp"
#include "vm/actor/builtin/v2/codes.hpp"
#include "vm/actor/cgo/actors.hpp"
#include "vm/exit_code/exit_code.hpp"
#include "vm/runtime/impl/runtime_impl.hpp"
#include "vm/runtime/runtime_error.hpp"

#include "vm/dvm/dvm.hpp"

namespace fc::vm::runtime {
  using actor::ActorVersion;
  using actor::getActorVersionForNetwork;
  using actor::isAccountActor;
  using actor::kEmptyObjectCid;
  using actor::kRewardAddress;
  using actor::kSendMethodNumber;
  using actor::kSystemActorAddress;
  using storage::hamt::HamtError;
  using version::getNetworkVersion;

  outcome::result<Address> resolveKey(StateTree &state_tree,
                                      IpldPtr ipld,
                                      const Address &address,
                                      bool allow_actor) {
    if (address.isKeyType()) {
      return address;
    }
    if (auto _actor{state_tree.get(address)}) {
      auto &actor{_actor.value()};
      if (actor.code == actor::builtin::v0::kAccountCodeCid) {
        if (auto _state{
                ipld->getCbor<actor::builtin::v0::account::AccountActorState>(
                    actor.head)}) {
          auto &key{_state.value().address};
          if (allow_actor || key.isKeyType()) {
            return key;
          }
        }
      }
      if (actor.code == actor::builtin::v2::kAccountCodeCid) {
        if (auto _state{
                ipld->getCbor<actor::builtin::v2::account::AccountActorState>(
                    actor.head)}) {
          auto &key{_state.value().address};
          if (allow_actor || key.isKeyType()) {
            return key;
          }
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
      apply.penalty = msg_gas_cost * tipset->getParentBaseFee();
      apply.receipt.exit_code = VMExitCode::kSysErrOutOfGas;
      return apply;
    }
    apply.penalty = message.gas_limit * tipset->getParentBaseFee();
    auto maybe_from = state_tree->get(message.from);
    if (!maybe_from) {
      if (maybe_from.error() == HamtError::kNotFound) {
        apply.receipt.exit_code = VMExitCode::kSysErrSenderInvalid;
        return apply;
      }
      return maybe_from.error();
    }
    auto &from = maybe_from.value();
    if (!isAccountActor(from.code)) {
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
    auto base_fee{tipset->getParentBaseFee()}, fee_cap{message.gas_fee_cap},
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
        used == 0  ? limit
        : over < 0 ? 0
                   : static_cast<GasAmount>(bigdiv(
                       BigInt{limit - used} * std::min(used, over), used))};
    if (gas_burned != 0) {
      OUTCOME_TRY(add_locked(actor::kBurntFundsActorAddress,
                             base_fee_pay * gas_burned));
      apply.penalty += (base_fee - base_fee_pay) * gas_burned;
    }
    BOOST_ASSERT_MSG(locked >= 0, "gas math wrong");
    OUTCOME_TRY(add_locked(message.from, locked));
    apply.receipt.exit_code = exit_code;
    apply.receipt.gas_used = used;

    dvm::onReceipt(apply.receipt);

    return apply;
  }

  outcome::result<MessageReceipt> Env::applyImplicitMessage(
      UnsignedMessage message) {
    auto execution = Execution::make(shared_from_this(), message);
    auto result = execution->send(message);
    if (result.has_error() && !isVMExitCode(result.error())) {
      return result.error();
    }
    MessageReceipt receipt{result.has_value()
                               ? VMExitCode::kOk
                               : VMExitCode{result.error().value()},
                           result.has_value() ? result.value() : Buffer{},
                           0};

    dvm::onReceipt(receipt);

    return receipt;
  }

  outcome::result<void> Execution::chargeGas(GasAmount amount) {
    dvm::onCharge(amount);

    gas_used += amount;
    if (gas_used > gas_limit) {
      gas_used = gas_limit;
      return VMExitCode::kSysErrOutOfGas;
    }
    return outcome::success();
  }

  std::shared_ptr<Execution> Execution::make(const std::shared_ptr<Env> &env,
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

    // Get correct version of actor to create
    CID account_code_cid_to_create;
    MethodNumber account_actor_create_method_number;
    switch (getActorVersionForNetwork(
        getNetworkVersion(static_cast<ChainEpoch>(env->epoch)))) {
      case ActorVersion::kVersion0:
        account_code_cid_to_create = actor::builtin::v0::kAccountCodeCid;
        account_actor_create_method_number =
            actor::builtin::v0::account::Construct::Number;
        break;
      case ActorVersion::kVersion2:
        account_code_cid_to_create = actor::builtin::v2::kAccountCodeCid;
        account_actor_create_method_number =
            actor::builtin::v2::account::Construct::Number;
        break;
    }

    OUTCOME_TRY(state_tree->set(
        id, {account_code_cid_to_create, actor::kEmptyObjectCid, {}, {}}));
    OUTCOME_TRY(params, actor::encodeActorParams(address));
    OUTCOME_TRY(sendWithRevert({id,
                                kSystemActorAddress,
                                {},
                                {},
                                {},
                                {},
                                account_actor_create_method_number,
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
    dvm::onReceipt(result, gas_used);
    return result;
  }

  outcome::result<InvocationOutput> Execution::send(
      const UnsignedMessage &message, GasAmount charge) {
    dvm::onSend(message);
    DVM_INDENT;

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
    OUTCOME_TRY(chargeGas(
        env->pricelist.onMethodInvocation(message.value, message.method)));
    OUTCOME_TRY(caller_id, state_tree->lookupId(message.from));
    auto _message{message};
    _message.from = caller_id;

    OUTCOME_TRY(to_id, state_tree->lookupId(message.to));
    if (getNetworkVersion(static_cast<ChainEpoch>(env->epoch))
        >= NetworkVersion::kVersion4) {
      _message.to = to_id;
    }

    if (message.value != 0) {
      if (message.value < 0) {
        return VMExitCode::kSysErrForbidden;
      }
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
      _message.from = caller_id;
      auto runtime = std::make_shared<RuntimeImpl>(
          shared_from_this(), _message, caller_id);
      return env->invoker->invoke(to_actor, runtime);
    }

    return outcome::success();
  }

  outcome::result<void> ChargingIpld::set(const CID &key, Value value) {
    auto execution{execution_.lock()};
    OUTCOME_TRY(execution->chargeGas(
        execution->env->pricelist.onIpldPut(value.size())));
    dvm::onIpldSet(key, value);
    return execution->env->ipld->set(key, std::move(value));
  }

  outcome::result<Ipld::Value> ChargingIpld::get(const CID &key) const {
    auto execution{execution_.lock()};
    OUTCOME_TRY(execution->chargeGas(execution->env->pricelist.onIpldGet()));
    OUTCOME_TRY(value, execution->env->ipld->get(key));
    return std::move(value);
  }
}  // namespace fc::vm::runtime
