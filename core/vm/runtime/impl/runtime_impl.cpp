/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/impl/runtime_impl.hpp"

#include "codec/cbor/cbor.hpp"
#include "vm/actor/account_actor.hpp"
#include "vm/message/message_codec.hpp"
#include "vm/runtime/gas_cost.hpp"
#include "vm/runtime/impl/actor_state_handle_impl.hpp"
#include "vm/runtime/runtime_error.hpp"

using fc::crypto::randomness::ChainEpoch;
using fc::crypto::randomness::Randomness;
using fc::crypto::randomness::Serialization;
using fc::primitives::BigInt;
using fc::primitives::address::Address;
using fc::primitives::address::Protocol;
using fc::storage::hamt::HamtError;
using fc::storage::ipfs::IpfsDatastore;
using fc::vm::actor::Actor;
using fc::vm::actor::CodeId;
using fc::vm::actor::MethodNumber;
using fc::vm::indices::Indices;
using fc::vm::message::UnsignedMessage;
using fc::vm::runtime::ActorStateHandle;
using fc::vm::runtime::ActorStateHandleImpl;
using fc::vm::runtime::InvocationOutput;
using fc::vm::runtime::Runtime;
using fc::vm::runtime::RuntimeError;
using fc::vm::runtime::RuntimeImpl;

RuntimeImpl::RuntimeImpl(
    std::shared_ptr<RandomnessProvider> randomness_provider,
    std::shared_ptr<IpfsDatastore> datastore,
    std::shared_ptr<StateTree> state_tree,
    std::shared_ptr<Indices> indices,
    std::shared_ptr<Invoker> invoker,
    std::shared_ptr<UnsignedMessage> message,
    ChainEpoch chain_epoch,
    Address immediate_caller,
    Address block_miner,
    BigInt gas_available,
    BigInt gas_used)
    : randomness_provider_{std::move(randomness_provider)},
      datastore_{std::move(datastore)},
      state_tree_{std::move(state_tree)},
      indices_{std::move(indices)},
      invoker_{std::move(invoker)},
      message_{std::move(message)},
      chain_epoch_{std::move(chain_epoch)},
      immediate_caller_{std::move(immediate_caller)},
      block_miner_{std::move(block_miner)},
      gas_available_{std::move(gas_available)},
      gas_used_{std::move(gas_used)} {}

ChainEpoch RuntimeImpl::getCurrentEpoch() const {
  return chain_epoch_;
}

Randomness RuntimeImpl::getRandomness(DomainSeparationTag tag,
                                      ChainEpoch epoch) const {
  return randomness_provider_->deriveRandomness(tag, Serialization{}, epoch);
}

Randomness RuntimeImpl::getRandomness(DomainSeparationTag tag,
                                      ChainEpoch epoch,
                                      Serialization seed) const {
  return randomness_provider_->deriveRandomness(tag, seed, epoch);
}

Address RuntimeImpl::getImmediateCaller() const {
  return immediate_caller_;
}

Address RuntimeImpl::getCurrentReceiver() const {
  return message_->to;
}

Address RuntimeImpl::getTopLevelBlockWinner() const {
  return block_miner_;
}

std::shared_ptr<ActorStateHandle> RuntimeImpl::acquireState() const {
  return std::make_shared<ActorStateHandleImpl>();
}

fc::outcome::result<BigInt> RuntimeImpl::getBalance(
    const Address &address) const {
  auto actor_state = state_tree_->get(address);
  if (!actor_state) {
    if (actor_state.error() == HamtError::NOT_FOUND) return BigInt(0);
    return actor_state.error();
  }
  return actor_state.value().balance;
}

BigInt RuntimeImpl::getValueReceived() const {
  return message_->value;
}

std::shared_ptr<Indices> RuntimeImpl::getCurrentIndices() const {
  return indices_;
}

fc::outcome::result<CodeId> RuntimeImpl::getActorCodeID(
    const Address &address) const {
  OUTCOME_TRY(actor_state, state_tree_->get(address));
  return actor_state.code;
}

fc::outcome::result<InvocationOutput> RuntimeImpl::send(
    Address to_address,
    MethodNumber method_number,
    MethodParams params,
    BigInt value) {
  OUTCOME_TRY(state_tree_->flush());

  // sender is a current 'to' in message
  OUTCOME_TRY(from_actor, state_tree_->get(message_->to));
  OUTCOME_TRY(to_actor, getOrCreateActor(to_address));

  auto message =
      std::make_shared<UnsignedMessage>(UnsignedMessage{message_->to,
                                                        to_address,
                                                        from_actor.nonce,
                                                        value,
                                                        gas_price_,
                                                        gas_available_,
                                                        method_number,
                                                        params});
  OUTCOME_TRY(serialized_message, fc::codec::cbor::encode(*message));
  OUTCOME_TRY(
      chargeGas(kOnChainMessageBaseGasCost
                + serialized_message.size() * kOnChainMessagePerByteGasCharge));

  BigInt gas_cost = gas_price_ * gas_available_;
  BigInt total_cost = gas_cost + value;
  if (from_actor.balance < total_cost) return RuntimeError::NOT_ENOUGH_FUNDS;

  // transfer
  if (value != 0) {
    OUTCOME_TRY(chargeGas(kSendTransferFundsGasCost));
    OUTCOME_TRY(transfer(from_actor, to_actor, value));
  }
  auto runtime = createRuntime(message);

  auto res = invoker_->invoke(to_actor, *runtime, method_number, params);
  if (!res) {
    OUTCOME_TRY(state_tree_->revert());
  } else {
    // transfer to miner
    OUTCOME_TRY(miner_actor, state_tree_->get(block_miner_));
    OUTCOME_TRY(transfer(from_actor, miner_actor, gas_used_ * gas_cost));

    OUTCOME_TRY(state_tree_->set(message_->to, from_actor));
    OUTCOME_TRY(state_tree_->set(to_address, to_actor));
    OUTCOME_TRY(state_tree_->set(block_miner_, miner_actor));

    OUTCOME_TRY(state_tree_->flush());
  }

  return res;
}

fc::outcome::result<void> RuntimeImpl::createActor(CodeId code_id,
                                                   const Address &address) {
  // May only be called by InitActor
  OUTCOME_TRY(actor_caller, state_tree_->get(immediate_caller_));
  if (actor_caller.code != actor::kInitCodeCid)
    return fc::outcome::failure(
        RuntimeError::CREATE_ACTOR_OPERATION_NOT_PERMITTED);

  Actor new_actor{code_id, ActorSubstateCID(), 0, 0};
  OUTCOME_TRY(state_tree_->registerNewAddress(address, new_actor));
  return fc::outcome::success();
}

fc::outcome::result<void> RuntimeImpl::deleteActor(const Address &address) {
  // May only be called by the actor itself, or by StoragePowerActor in the
  // case of StorageMinerActors
  OUTCOME_TRY(actor_caller, state_tree_->get(immediate_caller_));
  OUTCOME_TRY(actor_to_delete, state_tree_->get(address));
  if ((address == immediate_caller_)
      || (actor_caller.code == actor::kStoragePowerCodeCid
          && actor_to_delete.code == actor::kStorageMinerCodeCid)) {
    // TODO(a.chernyshov) FIL-137 implement state_tree remove if needed
    // return state_tree_->remove(address);
    return fc::outcome::failure(RuntimeError::UNKNOWN);
  }
  return fc::outcome::failure(
      RuntimeError::DELETE_ACTOR_OPERATION_NOT_PERMITTED);
}

std::shared_ptr<IpfsDatastore> RuntimeImpl::getIpfsDatastore() {
  return datastore_;
}

std::shared_ptr<UnsignedMessage> RuntimeImpl::getMessage() {
  return message_;
}

fc::outcome::result<void> RuntimeImpl::transfer(Actor &from,
                                                Actor &to,
                                                const BigInt &amount) {
  if (from.balance < amount) return RuntimeError::NOT_ENOUGH_FUNDS;
  from.balance = from.balance - amount;
  to.balance = to.balance + amount;
  return outcome::success();
}

fc::outcome::result<void> RuntimeImpl::chargeGas(const BigInt &amount) {
  gas_used_ = gas_used_ + amount;
  if (gas_available_ < gas_used_) return RuntimeError::NOT_ENOUGH_GAS;
  return outcome::success();
}

fc::outcome::result<Actor> RuntimeImpl::getOrCreateActor(
    const Address &address) {
  auto actor = state_tree_->get(address);
  if (!actor) {
    if (actor.error() != HamtError::NOT_FOUND) {
      return actor.error();
    }
    return fc::vm::actor::AccountActor::create(state_tree_, address);
  }
  return actor;
}

std::shared_ptr<Runtime> RuntimeImpl::createRuntime(
    const std::shared_ptr<UnsignedMessage> &message) const {
  return std::make_shared<RuntimeImpl>(randomness_provider_,
                                       datastore_,
                                       state_tree_,
                                       indices_,
                                       invoker_,
                                       message,
                                       chain_epoch_,
                                       immediate_caller_,
                                       block_miner_,
                                       gas_available_,
                                       gas_used_);
}
