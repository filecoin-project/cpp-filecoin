/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/impl/runtime_impl.hpp"

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
using fc::vm::runtime::ActorStateHandle;
using fc::vm::runtime::ActorStateHandleImpl;
using fc::vm::runtime::InvocationOutput;
using fc::vm::runtime::RuntimeError;
using fc::vm::runtime::RuntimeImpl;

RuntimeImpl::RuntimeImpl(
    std::shared_ptr<RandomnessProvider> randomness_provider,
    std::shared_ptr<IpfsDatastore> datastore,
    std::shared_ptr<StateTree> state_tree,
    std::shared_ptr<Indices> indices,
    ChainEpoch chain_epoch,
    Address immediate_caller,
    Address receiver,
    Address block_miner)
    : randomness_provider_{std::move(randomness_provider)},
      datastore_{std::move(datastore)},
      state_tree_{std::move(state_tree)},
      indices_{std::move(indices)},
      chain_epoch_{chain_epoch},
      immediate_caller_{std::move(immediate_caller)},
      receiver_{std::move(receiver)},
      block_miner_{std::move(block_miner)} {}

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
  return receiver_;
}

Address RuntimeImpl::getTopLevelBlockWinner() const {
  return block_miner_;
}

std::shared_ptr<ActorStateHandle> RuntimeImpl::acquireState() const {
  return std::make_shared<ActorStateHandleImpl>();
}

fc::outcome::result<BigInt> RuntimeImpl::getBalance(
    const Address &address) const {
  // TODO(a.chernyshov) add state_tree->has()
  auto actor_state = state_tree_->get(address);
  if (!actor_state) {
    if (actor_state.error() == HamtError::NOT_FOUND) return BigInt(0);
    return actor_state.error();
  }
  return actor_state.value().balance;
}

std::shared_ptr<Indices> RuntimeImpl::getCurrentIndices() const {
  return indices_;
}

fc::outcome::result<CodeId> RuntimeImpl::getActorCodeID(
    const Address &address) const {
  OUTCOME_TRY(actor_state, state_tree_->get(address));
  return actor_state.code;
}

InvocationOutput RuntimeImpl::send(Address to_address,
                                   MethodNumber method_number,
                                   MethodParams params,
                                   BigInt value) {
  // TODO (a.chernyshov) message is needed
  return InvocationOutput();
}

fc::outcome::result<void> RuntimeImpl::createActor(CodeId code_id,
                                                   const Address &address) {
  // May only be called by InitActor
  OUTCOME_TRY(actor_caller, state_tree_->get(immediate_caller_));
  if (actor_caller.code != actor::kInitCodeCid)
    return fc::outcome::failure(
        RuntimeError::CREATE_ACTOR_OPERATION_NOT_PERMITTED);

  Actor new_actor{code_id, ActorSubstateCID(common::kEmptyCid), 0, 0};
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
    // TODO(a.chernyshov) implement state_tree remove
    // return state_tree_->remove(address);
    return fc::outcome::failure(RuntimeError::UNKNOWN);
  }
  return fc::outcome::failure(
      RuntimeError::DELETE_ACTOR_OPERATION_NOT_PERMITTED);
}

std::shared_ptr<IpfsDatastore> RuntimeImpl::getIpfsDatastore() {
  return datastore_;
}
