/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/interpreter/interpreter.hpp"

#include "vm/actor/builtin/miner/miner_actor.hpp"
#include "vm/runtime/impl/runtime_impl.hpp"
#include "vm/state/impl/state_tree_impl.hpp"
#include "vm/actor/impl/invoker_impl.hpp"

namespace fc::vm::interpreter {
  using actor::Actor;
  using actor::builtin::miner::MinerActorState;
  using actor::builtin::miner::MinerInfo;
  using actor::InvokerImpl;
  using primitives::address::Address;
  using runtime::Env;
  using state::StateTreeImpl;

  outcome::result<Address> getMinerOwner(StateTreeImpl &state_tree, Address miner) {
    OUTCOME_TRY(actor, state_tree.get(miner));
    OUTCOME_TRY(state, state_tree.getStore()->getCbor<MinerActorState>(actor.head));
    OUTCOME_TRY(info, state_tree.getStore()->getCbor<MinerInfo>(state.info));
    return info.owner;
  }

  outcome::result<Result> interpret(const std::shared_ptr<IpfsDatastore> &ipld, const Tipset &tipset) {
    // TODO: check duplicate miner

    auto state_tree = std::make_shared<StateTreeImpl>(ipld, tipset.getParentStateRoot());
    // TODO: randomness{tipset}
    // TODO: indices
    auto env = std::make_shared<Env>(nullptr, state_tree, nullptr, std::make_shared<InvokerImpl>());

    for (auto &block : tipset.blks) {
      env->block_miner = block.miner;

      OUTCOME_TRY(miner_owner, getMinerOwner(*state_tree, block.miner));
      OUTCOME_TRY(miner_owner_actor, state_tree->get(miner_owner));
      // TODO: transfer block reward
      OUTCOME_TRY(state_tree->set(miner_owner, miner_owner_actor));

      // TODO: applyMessage miner SubmitElectionPoSt
    }

    for (auto &block : tipset.blks) {
      env->block_miner = block.miner;
      // TODO: process messages
    }

    // TODO: applyMessage cron EpochTick
  }
}  // namespace fc::vm::interpreter
