/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/interpreter/interpreter.hpp"

#include "storage/amt/amt.hpp"
#include "vm/actor/builtin/cron/cron_actor.hpp"
#include "vm/actor/builtin/miner/miner_actor.hpp"
#include "vm/actor/impl/invoker_impl.hpp"
#include "vm/runtime/impl/runtime_impl.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

namespace fc::vm::interpreter {
  using actor::Actor;
  using actor::builtin::cron::kEpochTickMethodNumber;
  using actor::builtin::miner::MinerActorState;
  using actor::builtin::miner::MinerInfo;
  using actor::builtin::miner::kSubmitElectionPoStMethodNumber;
  using actor::InvokerImpl;
  using actor::kSendMethodNumber;
  using actor::kSystemActorAddress;
  using actor::kCronAddress;
  using message::UnsignedMessage;
  using message::SignedMessage;
  using primitives::address::Address;
  using primitives::block::MsgMeta;
  using runtime::Env;
  using runtime::MessageReceipt;
  using runtime::RuntimeImpl;
  using state::StateTreeImpl;
  using storage::amt::Amt;

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
      OUTCOME_TRY(system_actor, state_tree->get(kSystemActorAddress));
      // TODO: block reward amount
      OUTCOME_TRY(RuntimeImpl::transfer(system_actor, miner_owner_actor, 0));
      OUTCOME_TRY(state_tree->set(kSystemActorAddress, system_actor));
      OUTCOME_TRY(state_tree->set(miner_owner, miner_owner_actor));

      OUTCOME_TRY(receipt, env->applyMessage(UnsignedMessage{
        block.miner,
        kSystemActorAddress,
        system_actor.nonce,
        0,
        0,
        10000000000,
        kSubmitElectionPoStMethodNumber,
        {},
      }));
      if (receipt.exit_code != 0) {
        // TODO: error
      }
    }

    std::vector<MessageReceipt> receipts;
    std::map<Address, Actor> actor_cache;
    for (auto &block : tipset.blks) {
      env->block_miner = block.miner;

      auto apply_message = [&](const UnsignedMessage &message) -> outcome::result<void> {
        auto actor_it = actor_cache.find(message.from);
        if (actor_it == actor_cache.end()) {
          OUTCOME_TRY(actor, state_tree->get(message.from));
          actor_it = actor_cache.insert(std::make_pair(message.from, actor)).first;
        }
        auto &actor = actor_it->second;

        if (actor.nonce != message.nonce) {
          return outcome::success();
        }
        ++actor.nonce;

        if (actor.balance < message.requiredFunds()) {
          return outcome::success();
        }
        actor.balance -= message.requiredFunds();

        OUTCOME_TRY(receipt, env->applyMessage(message));
        receipts.push_back(std::move(receipt));
        return outcome::success();
      };

      OUTCOME_TRY(meta, ipld->getCbor<MsgMeta>(block.messages));
      Amt{ipld, meta.bls_messages}.visit([&](auto, auto &cid_encoded) -> outcome::result<void> {
        OUTCOME_TRY(cid, codec::cbor::decode<CID>(cid_encoded));
        OUTCOME_TRY(message, ipld->getCbor<UnsignedMessage>(cid));
        return apply_message(message);
      });
      Amt{ipld, meta.secpk_messages}.visit([&](auto, auto &cid_encoded) -> outcome::result<void> {
        OUTCOME_TRY(cid, codec::cbor::decode<CID>(cid_encoded));
        OUTCOME_TRY(message, ipld->getCbor<SignedMessage>(cid));
        return apply_message(message.message);
      });
    }

    OUTCOME_TRY(cron_actor, state_tree->get(kCronAddress));
    OUTCOME_TRY(receipt, env->applyMessage(UnsignedMessage{
        kCronAddress,
        kCronAddress,
        cron_actor.nonce,
        0,
        0,
        1 << 30,
        kEpochTickMethodNumber,
        {},
      }));

    OUTCOME_TRY(new_state_root, state_tree->flush());

    Amt receipts_amt{ipld};
    for (auto i = 0u; i < receipts.size(); ++i) {
      OUTCOME_TRY(receipts_amt.set(i, receipts[i]));
    }
    OUTCOME_TRY(receipts_root, receipts_amt.flush());

    return Result{
      new_state_root,
      receipts_root,
    };
  }
}  // namespace fc::vm::interpreter
