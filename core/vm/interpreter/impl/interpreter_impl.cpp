/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/interpreter/impl/interpreter_impl.hpp"

#include "const.hpp"
#include "vm/actor/builtin/cron/cron_actor.hpp"
#include "vm/actor/builtin/reward/reward_actor.hpp"
#include "vm/actor/impl/invoker_impl.hpp"
#include "vm/runtime/impl/runtime_impl.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::vm::interpreter, InterpreterError, e) {
  using E = fc::vm::interpreter::InterpreterError;
  switch (e) {
    case E::kDuplicateMiner:
      return "Duplicate miner";
    case E::kMinerSubmitFailed:
      return "Miner submit failed";
    case E::kCronTickFailed:
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
  using message::SignedMessage;
  using message::UnsignedMessage;
  using primitives::TokenAmount;
  using primitives::block::MsgMeta;
  using primitives::tipset::MessageVisitor;
  using runtime::Env;
  using runtime::MessageReceipt;

  InterpreterImpl::InterpreterImpl(std::shared_ptr<Invoker> invoker)
      : invoker_{std::move(invoker)} {}

  outcome::result<Result> InterpreterImpl::interpret(
      const IpldPtr &ipld, const Tipset &tipset) const {
    if (tipset.height == 0) {
      return Result{
          tipset.getParentStateRoot(),
          tipset.getParentMessageReceipts(),
      };
    }
    return applyBlocks(ipld, tipset, {});
  }

  outcome::result<Result> InterpreterImpl::applyBlocks(
      const IpldPtr &ipld,
      const Tipset &tipset,
      std::vector<MessageReceipt> *all_receipts) const {
    auto on_receipt{[&](auto &receipt) {
      if (all_receipts) {
        all_receipts->push_back(receipt);
      }
      return outcome::success();
    }};

    if (hasDuplicateMiners(tipset.blks)) {
      return InterpreterError::kDuplicateMiner;
    }

    auto env = std::make_shared<Env>(invoker_, ipld, tipset);

    auto cron{[&]() -> outcome::result<void> {
      OUTCOME_TRY(receipt,
                  env->applyImplicitMessage(UnsignedMessage{
                      kCronAddress,
                      kSystemActorAddress,
                      {},
                      0,
                      0,
                      kBlockGasLimit * 10000,
                      EpochTick::Number,
                      {},
                  }));
      if (receipt.exit_code != VMExitCode::kOk) {
        return receipt.exit_code;
      }
      on_receipt(receipt);
      return outcome::success();
    }};

    if (tipset.height > 1) {
      OUTCOME_TRY(parent, tipset.loadParent(*ipld));
      for (auto epoch{parent.height + 1}; epoch < tipset.height; ++epoch) {
        env->tipset.height = epoch;
        OUTCOME_TRY(cron());
      }
      env->tipset.height = tipset.height;
    }

    adt::Array<MessageReceipt> receipts{ipld};
    MessageVisitor message_visitor{ipld};
    for (auto &block : tipset.blks) {
      AwardBlockReward::Params reward{
          block.miner, 0, 0, block.election_proof.win_count};
      OUTCOME_TRY(message_visitor.visit(
          block, [&](auto, auto bls, auto &cid) -> outcome::result<void> {
            UnsignedMessage message;
            OUTCOME_TRY(raw, ipld->get(cid));
            if (bls) {
              OUTCOME_TRYA(message, codec::cbor::decode<UnsignedMessage>(raw));
            } else {
              OUTCOME_TRY(signed_message,
                          codec::cbor::decode<SignedMessage>(raw));
              message = std::move(signed_message.message);
            }
            OUTCOME_TRY(apply, env->applyMessage(message, raw.size()));
            reward.penalty += apply.penalty;
            reward.gas_reward += apply.reward;
            on_receipt(apply.receipt);
            OUTCOME_TRY(receipts.append(std::move(apply.receipt)));
            return outcome::success();
          }));

      OUTCOME_TRY(reward_encoded, codec::cbor::encode(reward));
      OUTCOME_TRY(receipt,
                  env->applyImplicitMessage(UnsignedMessage{
                      kRewardAddress,
                      kSystemActorAddress,
                      {},
                      0,
                      0,
                      1 << 30,
                      AwardBlockReward::Number,
                      MethodParams{reward_encoded},
                  }));
      if (receipt.exit_code != VMExitCode::kOk) {
        return receipt.exit_code;
      }
      on_receipt(receipt);
    }

    OUTCOME_TRY(cron());

    OUTCOME_TRY(new_state_root, env->state_tree->flush());

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
    // TODO: TipsetKey from art-gor
    common::Buffer key;
    for (auto &cid : tipset.cids) {
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
