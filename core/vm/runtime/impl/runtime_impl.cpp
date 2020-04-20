/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/impl/runtime_impl.hpp"

#include "codec/cbor/cbor.hpp"
#include "proofs/proofs.hpp"
#include "vm/actor/builtin/account/account_actor.hpp"
#include "vm/runtime/gas_cost.hpp"
#include "vm/runtime/impl/actor_state_handle_impl.hpp"
#include "vm/runtime/runtime_error.hpp"

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
  using fc::vm::message::UnsignedMessage;

  RuntimeImpl::RuntimeImpl(std::shared_ptr<Execution> execution,
                           UnsignedMessage message,
                           ActorSubstateCID current_actor_state)
      : execution_{std::move(execution)},
        state_tree_{execution_->state_tree},
        message_{std::move(message)},
        current_actor_state_{std::move(current_actor_state)} {}

  ChainEpoch RuntimeImpl::getCurrentEpoch() const {
    return execution_->env->chain_epoch;
  }

  Randomness RuntimeImpl::getRandomness(DomainSeparationTag tag,
                                        ChainEpoch epoch) const {
    return execution_->env->randomness_provider->deriveRandomness(
        tag, Serialization{}, epoch);
  }

  Randomness RuntimeImpl::getRandomness(DomainSeparationTag tag,
                                        ChainEpoch epoch,
                                        Serialization seed) const {
    return execution_->env->randomness_provider->deriveRandomness(
        tag, seed, epoch);
  }

  Address RuntimeImpl::getImmediateCaller() const {
    return message_.from;
  }

  Address RuntimeImpl::getCurrentReceiver() const {
    return message_.to;
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
    return execution_->sendWithRevert(
        {to_address, message_.to, {}, value, {}, {}, method_number, params});
  }

  fc::outcome::result<void> RuntimeImpl::createActor(const Address &address,
                                                     const Actor &actor) {
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

  outcome::result<bool> RuntimeImpl::verifySignature(
      const Signature &signature,
      const Address &address,
      gsl::span<const uint8_t> data) {
    // TODO(turuslan): implement
    return RuntimeError::UNKNOWN;
  }

  fc::outcome::result<bool> RuntimeImpl::verifyPoSt(
      const PoStVerifyInfo &info) {
    PoStVerifyInfo preprocess_info = info;
    preprocess_info.randomness[31] = 0;
    return proofs::Proofs::verifyPoSt(preprocess_info);
  }

  fc::outcome::result<bool> RuntimeImpl::verifySeal(
      const SealVerifyInfo &info) {
    return proofs::Proofs::verifySeal(info);
  }

  fc::outcome::result<fc::CID> RuntimeImpl::computeUnsealedSectorCid(
      RegisteredProof type, const std::vector<PieceInfo> &pieces) {
    // TODO(turuslan): FIL-112 implement
    return RuntimeError::UNKNOWN;
  }

  fc::outcome::result<bool> RuntimeImpl::verifyConsensusFault(
      const BlockHeader &block_header_1, const BlockHeader &block_header_2) {
    // TODO(a.chernyshov): implement
    return RuntimeError::UNKNOWN;
  }

  fc::outcome::result<void> RuntimeImpl::chargeGas(GasAmount amount) {
    return execution_->chargeGas(amount);
  }

  std::shared_ptr<Runtime> RuntimeImpl::createRuntime(
      const UnsignedMessage &message,
      const ActorSubstateCID &current_actor_state) const {
    return std::make_shared<RuntimeImpl>(
        execution_, message, current_actor_state);
  }

}  // namespace fc::vm::runtime
