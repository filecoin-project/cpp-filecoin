/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/vm/runtime/impl/runtime_impl.hpp"

#include "filecoin/codec/cbor/cbor.hpp"
#include "filecoin/vm/actor/builtin/account/account_actor.hpp"
#include "filecoin/vm/runtime/gas_cost.hpp"
#include "filecoin/vm/runtime/impl/actor_state_handle_impl.hpp"
#include "filecoin/vm/runtime/runtime_error.hpp"

namespace fc::vm::runtime {

  using fc::crypto::randomness::ChainEpoch;
  using fc::crypto::randomness::Randomness;
  using fc::crypto::randomness::Serialization;
  using fc::primitives::BigInt;
  using fc::primitives::address::Address;
  using fc::primitives::address::Protocol;
  using fc::storage::hamt::HamtError;
  using fc::storage::ipfs::IpfsDatastore;
  using fc::vm::actor::Actor;
  using fc::vm::actor::ActorSubstateCID;
  using fc::vm::actor::CodeId;
  using fc::vm::actor::MethodNumber;
  using fc::vm::actor::builtin::account::AccountActor;
  using fc::vm::indices::Indices;
  using fc::vm::message::UnsignedMessage;

  RuntimeImpl::RuntimeImpl(std::shared_ptr<Env> env,
                           UnsignedMessage message,
                           Address immediate_caller,
                           BigInt gas_available,
                           BigInt gas_used,
                           ActorSubstateCID current_actor_state)
      : env_{std::move(env)},
        state_tree_{env_->state_tree},
        message_{std::move(message)},
        immediate_caller_{std::move(immediate_caller)},
        gas_available_{std::move(gas_available)},
        gas_used_{std::move(gas_used)},
        current_actor_state_{std::move(current_actor_state)} {}

  ChainEpoch RuntimeImpl::getCurrentEpoch() const {
    return env_->chain_epoch;
  }

  Randomness RuntimeImpl::getRandomness(DomainSeparationTag tag,
                                        ChainEpoch epoch) const {
    return env_->randomness_provider->deriveRandomness(
        tag, Serialization{}, epoch);
  }

  Randomness RuntimeImpl::getRandomness(DomainSeparationTag tag,
                                        ChainEpoch epoch,
                                        Serialization seed) const {
    return env_->randomness_provider->deriveRandomness(tag, seed, epoch);
  }

  Address RuntimeImpl::getImmediateCaller() const {
    return immediate_caller_;
  }

  Address RuntimeImpl::getCurrentReceiver() const {
    return message_.to;
  }

  Address RuntimeImpl::getTopLevelBlockWinner() const {
    return env_->block_miner;
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
    return message_.value;
  }

  std::shared_ptr<Indices> RuntimeImpl::getCurrentIndices() const {
    return env_->indices;
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
    auto message = UnsignedMessage{to_address,
                                   message_.to,
                                   0,  // unused
                                   value,
                                   0,  // unused
                                   gas_available_,
                                   method_number,
                                   params};
    return env_->send(gas_used_, immediate_caller_, message);
  }

  fc::outcome::result<void> RuntimeImpl::createActor(const Address &address,
                                                     const Actor &actor) {
    // May only be called by InitActor
    OUTCOME_TRY(actor_caller, state_tree_->get(immediate_caller_));
    if (actor_caller.code != actor::kInitCodeCid)
      return fc::outcome::failure(
          RuntimeError::CREATE_ACTOR_OPERATION_NOT_PERMITTED);
    OUTCOME_TRY(state_tree_->set(address, actor));
    return fc::outcome::success();
  }

  fc::outcome::result<void> RuntimeImpl::deleteActor() {
    // TODO(a.chernyshov) FIL-137 implement state_tree remove if needed
    // return state_tree_->remove(address);
    return fc::outcome::failure(RuntimeError::UNKNOWN);
  }

  std::shared_ptr<IpfsDatastore> RuntimeImpl::getIpfsDatastore() {
    // TODO(turuslan): FIL-131 charging store
    return state_tree_->getStore();
  }

  std::reference_wrapper<const UnsignedMessage> RuntimeImpl::getMessage() {
    return message_;
  }

  ActorSubstateCID RuntimeImpl::getCurrentActorState() {
    return current_actor_state_;
  }

  fc::outcome::result<void> RuntimeImpl::commit(
      const ActorSubstateCID &new_state) {
    OUTCOME_TRY(chargeGas(kCommitGasCost));
    current_actor_state_ = new_state;
    return outcome::success();
  }

  BigInt RuntimeImpl::gasUsed() const {
    return gas_used_;
  }

  fc::outcome::result<void> RuntimeImpl::transfer(Actor &from,
                                                  Actor &to,
                                                  const BigInt &amount) {
    if (from.balance < amount) return RuntimeError::NOT_ENOUGH_FUNDS;
    from.balance = from.balance - amount;
    to.balance = to.balance + amount;
    return outcome::success();
  }

  fc::outcome::result<Address> RuntimeImpl::resolveAddress(
      const Address &address) {
    return state_tree_->lookupId(address);
  }

  fc::outcome::result<bool> RuntimeImpl::verifyPoSt(
      uint64_t sector_size, const PoStVerifyInfo &info) {
    // TODO(turuslan): FIL-160 connect verifyPoSt from proofs
    return RuntimeError::UNKNOWN;
  }

  fc::outcome::result<bool> RuntimeImpl::verifySeal(
      uint64_t sector_size, const SealVerifyInfo &info) {
    // TODO(turuslan): FIL-160 connect verifySeal from proofs
    return RuntimeError::UNKNOWN;
  }

  fc::outcome::result<bool> RuntimeImpl::verifyConsensusFault(
      const BlockHeader &block_header_1, const BlockHeader &block_header_2) {
    // TODO(a.chernyshov): implement
    return RuntimeError::UNKNOWN;
  }

  fc::outcome::result<void> RuntimeImpl::chargeGas(const BigInt &amount) {
    gas_used_ = gas_used_ + amount;
    if (gas_available_ != kInfiniteGas && gas_available_ < gas_used_) {
      return RuntimeError::NOT_ENOUGH_GAS;
    }
    return outcome::success();
  }

  fc::outcome::result<Actor> RuntimeImpl::getOrCreateActor(
      const Address &address) {
    auto actor = state_tree_->get(address);
    if (!actor) {
      if (actor.error() != HamtError::NOT_FOUND) {
        return actor.error();
      }
      return AccountActor::create(state_tree_, address);
    }
    return actor;
  }

  std::shared_ptr<Runtime> RuntimeImpl::createRuntime(
      const UnsignedMessage &message,
      const ActorSubstateCID &current_actor_state) const {
    return std::make_shared<RuntimeImpl>(env_,
                                         message,
                                         immediate_caller_,
                                         gas_available_,
                                         gas_used_,
                                         current_actor_state);
  }

}  // namespace fc::vm::runtime
