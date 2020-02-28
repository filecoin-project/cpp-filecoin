/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/interpreter/interpreter.hpp"

#include "crypto/randomness/randomness_provider.hpp"
#include "storage/amt/amt.hpp"
#include "vm/actor/builtin/cron/cron_actor.hpp"
#include "vm/actor/builtin/miner/miner_actor.hpp"
#include "vm/actor/impl/invoker_impl.hpp"
#include "vm/message/message_codec.hpp"
#include "vm/runtime/gas_cost.hpp"
#include "vm/runtime/impl/runtime_impl.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::vm::interpreter, InterpreterError, e) {
  using E = fc::vm::interpreter::InterpreterError;
  switch (e) {
    case E::DUPLICATE_MINER:
      return "Duplicate miner";
    case E::MINER_SUBMIT_FAILED:
      return "Miner submit failed";
    case E::CRON_TICK_FAILED:
      return "Cron tick failed";
  }
}

namespace fc::vm::interpreter {
  using actor::Actor;
  using actor::InvokerImpl;
  using actor::kCronAddress;
  using actor::kSystemActorAddress;
  using actor::builtin::cron::EpochTick;
  using actor::builtin::miner::kSubmitElectionPoStMethodNumber;
  using actor::builtin::miner::MinerActorState;
  using actor::builtin::miner::MinerInfo;
  using crypto::randomness::RandomnessProvider;
  using message::SignedMessage;
  using message::UnsignedMessage;
  using primitives::address::Address;
  using primitives::block::BlockHeader;
  using primitives::block::MsgMeta;
  using runtime::Env;
  using runtime::kInfiniteGas;
  using runtime::MessageReceipt;
  using runtime::RuntimeImpl;
  using state::StateTreeImpl;
  using storage::amt::Amt;

  bool hasDuplicateMiners(const std::vector<BlockHeader> &blocks) {
    std::set<Address> set;
    for (auto &block : blocks) {
      if (!set.insert(block.miner).second) {
        return true;
      }
    }
    return false;
  }

  outcome::result<Address> getMinerOwner(StateTreeImpl &state_tree,
                                         const Address &miner) {
    OUTCOME_TRY(actor, state_tree.get(miner));
    OUTCOME_TRY(state,
                state_tree.getStore()->getCbor<MinerActorState>(actor.head));
    return state.info.owner;
  }

  outcome::result<Result> interpret(const std::shared_ptr<IpfsDatastore> &ipld,
                                    const Tipset &tipset,
                                    const std::shared_ptr<Indices> &indices) {
    if (hasDuplicateMiners(tipset.blks)) {
      return InterpreterError::DUPLICATE_MINER;
    }

    auto state_tree =
        std::make_shared<StateTreeImpl>(ipld, tipset.getParentStateRoot());
    // TODO(turuslan): FIL-146 randomness from tipset
    std::shared_ptr<RandomnessProvider> randomness;
    auto env = std::make_shared<Env>(randomness,
                                     state_tree,
                                     indices,
                                     std::make_shared<InvokerImpl>(),
                                     tipset.height,
                                     Address{});

    for (auto &block : tipset.blks) {
      env->block_miner = block.miner;

      OUTCOME_TRY(miner_owner, getMinerOwner(*state_tree, block.miner));
      OUTCOME_TRY(miner_owner_actor, state_tree->get(miner_owner));
      OUTCOME_TRY(system_actor, state_tree->get(kSystemActorAddress));
      // TODO(turuslan): block reward amount
      OUTCOME_TRY(RuntimeImpl::transfer(system_actor, miner_owner_actor, 0));
      OUTCOME_TRY(state_tree->set(kSystemActorAddress, system_actor));
      OUTCOME_TRY(state_tree->set(miner_owner, miner_owner_actor));

      OUTCOME_TRY(receipt,
                  env->applyMessage(UnsignedMessage{
                      block.miner,
                      kSystemActorAddress,
                      system_actor.nonce,
                      0,
                      0,
                      kInfiniteGas,
                      kSubmitElectionPoStMethodNumber,
                      {},
                  }));
      if (receipt.exit_code != 0) {
        return InterpreterError::MINER_SUBMIT_FAILED;
      }
    }

    std::vector<MessageReceipt> receipts;
    std::map<Address, Actor> actor_cache;
    for (auto &block : tipset.blks) {
      env->block_miner = block.miner;

      auto apply_message =
          [&](const UnsignedMessage &message) -> outcome::result<void> {
        auto actor_it = actor_cache.find(message.from);
        if (actor_it == actor_cache.end()) {
          OUTCOME_TRY(actor, state_tree->get(message.from));
          actor_it =
              actor_cache.insert(std::make_pair(message.from, actor)).first;
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
      OUTCOME_TRY(
          Amt(ipld, meta.bls_messages)
              .visit([&](auto, auto &cid_encoded) -> outcome::result<void> {
                OUTCOME_TRY(cid, codec::cbor::decode<CID>(cid_encoded));
                OUTCOME_TRY(message, ipld->getCbor<UnsignedMessage>(cid));
                return apply_message(message);
              }));
      OUTCOME_TRY(
          Amt(ipld, meta.secpk_messages)
              .visit([&](auto, auto &cid_encoded) -> outcome::result<void> {
                OUTCOME_TRY(cid, codec::cbor::decode<CID>(cid_encoded));
                OUTCOME_TRY(message, ipld->getCbor<SignedMessage>(cid));
                return apply_message(message.message);
              }));
    }

    OUTCOME_TRY(cron_actor, state_tree->get(kCronAddress));
    OUTCOME_TRY(receipt,
                env->applyMessage(UnsignedMessage{
                    kCronAddress,
                    kCronAddress,
                    cron_actor.nonce,
                    0,
                    0,
                    kInfiniteGas,
                    EpochTick::Number,
                    {},
                }));
    if (receipt.exit_code != 0) {
      return InterpreterError::CRON_TICK_FAILED;
    }

    OUTCOME_TRY(new_state_root, state_tree->flush());

    Amt receipts_amt{ipld};
    for (auto i = 0u; i < receipts.size(); ++i) {
      OUTCOME_TRY(receipts_amt.setCbor(i, receipts[i]));
    }
    OUTCOME_TRY(receipts_amt.flush());

    return Result{
        new_state_root,
        receipts_amt.cid(),
    };
  }
}  // namespace fc::vm::interpreter
