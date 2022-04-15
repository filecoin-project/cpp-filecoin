/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/env.hpp"

#include "cbor_blake/cid.hpp"
#include "codec/cbor/light_reader/cid.hpp"
#include "common/prometheus/metrics.hpp"
#include "common/prometheus/since.hpp"
#include "vm/actor/builtin/v0/miner/miner_actor.hpp"
#include "vm/actor/cgo/actors.hpp"
#include "vm/exit_code/exit_code.hpp"
#include "vm/runtime/impl/runtime_impl.hpp"
#include "vm/runtime/runtime_error.hpp"
#include "vm/toolchain/toolchain.hpp"

#include "vm/dvm/dvm.hpp"

namespace fc::vm::runtime {
  using actor::ActorVersion;
  using actor::kConstructorMethodNumber;
  using actor::kEmptyObjectCid;
  using actor::kRewardAddress;
  using actor::kSendMethodNumber;
  using actor::kSystemActorAddress;
  using toolchain::Toolchain;
  using version::getNetworkVersion;

  auto &metricVmApplyCount() {
    static auto &x{prometheus::BuildCounter()
                       .Name("lotus_vm_applied")
                       .Help("Counter for messages (including internal "
                             "messages) processed by the VM")
                       .Register(prometheusRegistry())
                       .Add({})};
    return x;
  }

  IpldBuffered::IpldBuffered(IpldPtr ipld) : ipld{std::move(ipld)} {}

  outcome::result<void> IpldBuffered::flush(const CID &root) {
    assert(!flushed);
    flushed = true;

    static auto &metricTime{prometheus::BuildCounter()
                                .Name("lotus_vm_flush_copy_ms")
                                .Help("Time spent in VM Flush Copy")
                                .Register(prometheusRegistry())
                                .Add({})};
    static auto &metricCount{prometheus::BuildCounter()
                                 .Name("lotus_vm_flush_copy_count")
                                 .Help("Number of copied objects")
                                 .Register(prometheusRegistry())
                                 .Add({})};
    const Since since;

    assert(isCbor(root));
    auto _root{*asBlake(root)};
    assert(write.count(_root));
    std::vector<const CbCid *> queue{&_root};
    std::set<CbCid> visited{_root};
    size_t next{};
    while (next < queue.size()) {
      const auto &key{*queue[next++]};
      BytesIn value{write.at(key)};
      BytesIn _cid;
      while (codec::cbor::findCid(_cid, value)) {
        const CbCid *cid = nullptr;
        if (codec::cbor::light_reader::readCborBlake(cid, _cid)) {
          if (auto it{write.find(*cid)};
              it != write.end() && visited.emplace(*cid).second) {
            queue.push_back(&it->first);
          }
        }
      }
    }
    for (auto it{queue.rbegin()}; it != queue.rend(); ++it) {
      const auto &key{**it};
      OUTCOME_TRY(ipld->set(CID{key}, std::move(write.at(key))));
    }
    write.clear();

    metricTime.Increment(since.ms());
    metricCount.Increment(queue.size());

    return outcome::success();
  }

  outcome::result<bool> IpldBuffered::contains(const CID &cid) const {
    if (auto it{write.find(*asBlake(cid))}; it != write.end()) {
      return true;
    }
    return ipld->contains(cid).value();
  }

  outcome::result<void> IpldBuffered::set(const CID &cid, BytesCow &&value) {
    assert(isCbor(cid));
    write.emplace(*asBlake(cid), value.into());
    return outcome::success();
  }

  outcome::result<Ipld::Value> IpldBuffered::get(const CID &cid) const {
    if (isCbor(cid)) {
      if (auto it{write.find(*asBlake(cid))}; it != write.end()) {
        return it->second;
      }
      if (auto r{ipld->get(cid)}) {
        return r;
      }
    }
    spdlog::error("IpldBuffered.get: {}", common::hex_lower(*asBlake(cid)));
    return storage::ipfs::IpfsDatastoreError::kNotFound;
  }

  outcome::result<std::shared_ptr<Env>> Env::make(
      const EnvironmentContext &env_context,
      TsBranchPtr ts_branch,
      const TokenAmount &base_fee,
      const CID &state,
      ChainEpoch epoch) {
    auto env{std::make_shared<Env>()};
    env->ipld = std::make_shared<IpldBuffered>(env_context.ipld);
    env->state_tree = std::make_shared<StateTreeImpl>(env->ipld, state);
    env->env_context = env_context;
    env->epoch = epoch;
    env->ts_branch = std::move(ts_branch);
    env->base_state = state;
    env->base_fee = base_fee;
    env->pricelist = Pricelist{env->epoch};
    env->ipld->actor_version = actorVersion(epoch);
    if (env_context.circulating) {
      OUTCOME_TRYA(
          env->base_circulating,
          env_context.circulating->circulating(env->state_tree, epoch));
    }
    return env;
  }

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  outcome::result<ApplyRet> Env::applyMessage(const UnsignedMessage &message,
                                              size_t size) {
    auto BOOST_OUTCOME_TRY_UNIQUE_NAME{
        gsl::finally([] { metricVmApplyCount().Increment(); })};

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
    ApplyRet apply;
    auto msg_gas_cost{pricelist.onChainMessage(size)};
    if (msg_gas_cost > message.gas_limit) {
      apply.penalty = msg_gas_cost * base_fee;
      apply.receipt.exit_code = VMExitCode::kSysErrOutOfGas;
      return apply;
    }
    apply.penalty = message.gas_limit * base_fee;
    OUTCOME_TRY(maybe_from, state_tree->tryGet(message.from));
    if (!maybe_from) {
      apply.receipt.exit_code = VMExitCode::kSysErrSenderInvalid;
      return apply;
    }
    auto &from = maybe_from.value();
    const auto network_version = getNetworkVersion(epoch);
    const auto address_matcher =
        Toolchain::createAddressMatcher(network_version);
    if (!address_matcher->isAccountActor(from.code)) {
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

    state_tree->txBegin();
    auto BOOST_OUTCOME_TRY_UNIQUE_NAME{
        gsl::finally([&] { state_tree->txEnd(); })};
    auto result{execution->send(message, msg_gas_cost)};
    OUTCOME_TRY(exit_code, asExitCode(result));
    if (exit_code == VMExitCode::kFatal) {
      return result.error();
    }
    if (result) {
      auto &ret{result.value()};
      if (!ret.empty()) {
        auto charge =
            execution->chargeGas(pricelist.onChainReturnValue(ret.size()));
        catchAbort(charge, network_version);
        OUTCOME_TRYA(exit_code, asExitCode(charge));
        if (charge) {
          apply.receipt.return_value = std::move(ret);
        }
      }
    }
    if (exit_code != VMExitCode::kOk) {
      state_tree->txRevert();
    }
    auto limit{message.gas_limit};
    auto &used{execution->gas_used};
    if (used < 0) {
      used = 0;
    }
    auto no_fee{false};
    if (network_version <= NetworkVersion::kVersion12
        && epoch > kUpgradeClausHeight && exit_code == VMExitCode::kOk
        && message.method
               == vm::actor::builtin::v0::miner::SubmitWindowedPoSt::Number) {
      OUTCOME_TRY(to, state_tree->tryGet(message.to));
      if (to) {
        no_fee = address_matcher->isStorageMinerActor(to->code);
      }
    }
    BOOST_ASSERT_MSG(used <= limit, "runtime charged gas over limit");
    auto fee_cap{message.gas_fee_cap};
    auto base_fee_pay{std::min(base_fee, fee_cap)};
    apply.penalty = base_fee > fee_cap ? TokenAmount{base_fee - fee_cap} * used
                                       : TokenAmount{0};
    if (!no_fee) {
      OUTCOME_TRY(
          add_locked(actor::kBurntFundsActorAddress, base_fee_pay * used));
    }
    apply.reward =
        std::min(message.gas_premium, TokenAmount{fee_cap - base_fee_pay})
        * limit;
    OUTCOME_TRY(add_locked(kRewardAddress, apply.reward));
    auto over{limit - 11 * used / 10};
    auto gas_burned{
        used == 0
            ? limit
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
      const UnsignedMessage &message) {
    auto BOOST_OUTCOME_TRY_UNIQUE_NAME{
        gsl::finally([] { metricVmApplyCount().Increment(); })};

    auto execution = Execution::make(shared_from_this(), message);
    auto result = execution->send(message);
    MessageReceipt receipt;
    OUTCOME_TRYA(receipt.exit_code, asExitCode(result));
    if (result) {
      receipt.return_value = std::move(result.value());
    }

    dvm::onReceipt(receipt);

    return receipt;
  }

  outcome::result<CID> Env::flush() {
    OUTCOME_TRY(root, state_tree->flush());
    OUTCOME_TRY(ipld->flush(root));
    return std::move(root);
  }

  outcome::result<void> Execution::chargeGas(GasAmount amount) {
    dvm::onCharge(amount);

    gas_used += amount;
    if (gas_used > gas_limit) {
      gas_used = gas_limit;
      return asAbort(VMExitCode::kSysErrOutOfGas);
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
    const auto network_version = getNetworkVersion(env->epoch);
    OUTCOME_TRY(
        catchAbort(chargeGas(env->pricelist.onCreateActor()), network_version));
    OUTCOME_TRY(id, state_tree->registerNewAddress(address));
    if (!address.isKeyType()) {
      return VMExitCode::kSysErrInvalidReceiver;
    }

    // Get correct version of actor to create
    const auto address_matcher =
        Toolchain::createAddressMatcher(network_version);
    CID account_code_cid_to_create = address_matcher->getAccountCodeId();
    MethodNumber account_actor_create_method_number = kConstructorMethodNumber;

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
    state_tree->txBegin();
    auto BOOST_OUTCOME_TRY_UNIQUE_NAME{
        gsl::finally([&] { state_tree->txEnd(); })};
    auto result = send(message);
    if (!result) {
      state_tree->txRevert();
      return result.error();
    }
    dvm::onReceipt(result, gas_used);
    return result;
  }

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  outcome::result<InvocationOutput> Execution::send(
      const UnsignedMessage &message, GasAmount charge) {
    dvm::onSend(message);
    DVM_INDENT;

    static auto &metric{prometheus::BuildCounter()
                            .Name("lotus_vm_sends")
                            .Help("Counter for sends processed by the VM")
                            .Register(prometheusRegistry())
                            .Add({})};
    auto BOOST_OUTCOME_TRY_UNIQUE_NAME{
        gsl::finally([] { metric.Increment(); })};

    const auto network_version = getNetworkVersion(env->epoch);
    OUTCOME_TRY(catchAbort(chargeGas(charge), network_version));
    Actor to_actor;
    OUTCOME_TRY(maybe_to_actor, state_tree->tryGet(message.to));
    if (!maybe_to_actor) {
      OUTCOME_TRY(account_actor, tryCreateAccountActor(message.to));
      to_actor = account_actor;
    } else {
      to_actor = maybe_to_actor.value();
    }
    dvm::onSendTo(to_actor.code);
    OUTCOME_TRY(catchAbort(chargeGas(env->pricelist.onMethodInvocation(
                               message.value, message.method)),
                           network_version));
    OUTCOME_TRY(caller_id, state_tree->lookupId(message.from));
    auto _message{message};
    _message.from = caller_id;

    OUTCOME_TRY(to_id, state_tree->lookupId(message.to));
    if (network_version >= NetworkVersion::kVersion4) {
      _message.to = to_id;
    }

    if (message.value != 0) {
      if (message.value < 0) {
        return VMExitCode::kSysErrForbidden;
      }
      if (to_id != caller_id || network_version >= NetworkVersion::kVersion15) {
        OUTCOME_TRY(from_actor, state_tree->get(caller_id));
        if (from_actor.balance < message.value) {
          return VMExitCode::kSysErrInsufficientFunds;
        }
        if (to_id != caller_id) {
          from_actor.balance -= message.value;
          to_actor.balance += message.value;
          OUTCOME_TRY(state_tree->set(caller_id, from_actor));
          OUTCOME_TRY(state_tree->set(to_id, to_actor));
        }
      }
    }

    if (message.method != kSendMethodNumber) {
      _message.from = caller_id;
      auto runtime = std::make_shared<RuntimeImpl>(
          shared_from_this(), _message, caller_id);
      auto result = env->env_context.invoker->invoke(to_actor, runtime);
      catchAbort(result, network_version);
      return result;
    }

    return outcome::success();
  }

  outcome::result<void> ChargingIpld::set(const CID &key, BytesCow &&value) {
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
    dvm::onIpldGet(key, value);
    return std::move(value);
  }
}  // namespace fc::vm::runtime
