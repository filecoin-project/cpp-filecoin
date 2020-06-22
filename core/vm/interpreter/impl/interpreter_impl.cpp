/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/interpreter/impl/interpreter_impl.hpp"

#include "crypto/randomness/randomness_provider.hpp"
#include "vm/actor/builtin/cron/cron_actor.hpp"
#include "vm/actor/builtin/reward/reward_actor.hpp"
#include "vm/actor/impl/invoker_impl.hpp"
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
  using actor::kRewardAddress;
  using actor::kSystemActorAddress;
  using actor::MethodParams;
  using actor::builtin::cron::EpochTick;
  using actor::builtin::reward::AwardBlockReward;
  using crypto::randomness::RandomnessProvider;
  using message::SignedMessage;
  using message::UnsignedMessage;
  using primitives::TokenAmount;
  using primitives::block::MsgMeta;
  using primitives::tipset::MessageVisitor;
  using runtime::Env;
  using runtime::kInfiniteGas;
  using runtime::MessageReceipt;

  outcome::result<Result> InterpreterImpl::interpret(
      const IpldPtr &ipld, const Tipset &tipset) const {
    if (tipset.height() == 0) {
      return Result{
          tipset.getParentStateRoot(),
          tipset.getParentMessageReceipts(),
      };
    }

    if (hasDuplicateMiners(tipset.blks)) {
      return InterpreterError::DUPLICATE_MINER;
    }

    auto state_tree = std::make_shared<state::StateTreeImpl>(
        ipld, tipset.getParentStateRoot());
    // TODO(turuslan): FIL-146 randomness from tipset
    std::shared_ptr<RandomnessProvider> randomness;
    auto env = std::make_shared<Env>(
        randomness, state_tree, std::make_shared<InvokerImpl>(), tipset.height());

    adt::Array<MessageReceipt> receipts{ipld};
    MessageVisitor message_visitor{ipld};
    for (auto &block : tipset.blks) {
      AwardBlockReward::Params reward{block.miner, 0, 0, 1};
      OUTCOME_TRY(message_visitor.visit(
          block, [&](auto, auto bls, auto &cid) -> outcome::result<void> {
            UnsignedMessage message;
            if (bls) {
              OUTCOME_TRYA(message, ipld->getCbor<UnsignedMessage>(cid));
            } else {
              OUTCOME_TRY(signed_message, ipld->getCbor<SignedMessage>(cid));
              message = std::move(signed_message.message);
            }
            TokenAmount penalty;
            OUTCOME_TRY(receipt, env->applyMessage(message, penalty));
            reward.penalty += penalty;
            OUTCOME_TRY(receipts.append(std::move(receipt)));
            return outcome::success();
          }));

      OUTCOME_TRY(reward_encoded, codec::cbor::encode(reward));
      OUTCOME_TRY(env->applyImplicitMessage(UnsignedMessage{
          0,
          kRewardAddress,
          kSystemActorAddress,
          {},
          0,
          0,
          kInfiniteGas,
          AwardBlockReward::Number,
          MethodParams{reward_encoded},
      }));
    }

    OUTCOME_TRY(env->applyImplicitMessage(UnsignedMessage{
        0,
        kCronAddress,
        kSystemActorAddress,
        {},
        0,
        0,
        kInfiniteGas,
        EpochTick::Number,
        {},
    }));

    OUTCOME_TRY(new_state_root, state_tree->flush());

    OUTCOME_TRY(Ipld::flush(receipts));

    return Result{
        new_state_root,
        receipts.amt.cid(),
    };
  }

  bool InterpreterImpl::hasDuplicateMiners(
      const std::vector<BlockHeader> &blocks) const {
    std::set<primitives::address::Address> set;
    for (auto &block : blocks) {
      if (!set.insert(block.miner).second) {
        return true;
      }
    }
    return false;
  }

  outcome::result<Result> CachedInterpreter::interpret(
      const IpldPtr &ipld, const Tipset &tipset) const {
    // TODO: TipsetKey from arg-gor
    common::Buffer key;
    for (auto &cid : tipset.key.cids()) {
      OUTCOME_TRY(encoded, cid.toBytes());
      key.put(encoded);
    }

    if (store->contains(key)) {
      OUTCOME_TRY(raw, store->get(key));
      return codec::cbor::decode<Result>(raw);
    }
    OUTCOME_TRY(result, interpreter->interpret(ipld, tipset));
    OUTCOME_TRY(raw, codec::cbor::encode(result));
    OUTCOME_TRY(store->put(key, raw));
    return std::move(result);
  }
}  // namespace fc::vm::interpreter
