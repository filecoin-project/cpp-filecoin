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

  using primitives::block::MsgMeta;
  using runtime::Env;
  using runtime::kInfiniteGas;
  using runtime::MessageReceipt;
  using runtime::RuntimeImpl;

  using storage::amt::Amt;

  outcome::result<Result> InterpreterImpl::interpret(
      const std::shared_ptr<IpfsDatastore> &ipld, const Tipset &tipset) const {
    if (tipset.height == 0) {
      return Result{
          tipset.getParentStateRoot(),
          tipset.getParentMessageReceipts(),
      };
    }

    if (hasDuplicateMiners(tipset.blks)) {
      return InterpreterError::DUPLICATE_MINER;
    }

    auto state_tree =
        std::make_shared<StateTreeImpl>(ipld, tipset.getParentStateRoot());
    // TODO(turuslan): FIL-146 randomness from tipset
    std::shared_ptr<RandomnessProvider> randomness;
    auto env = std::make_shared<Env>(
        randomness, state_tree, std::make_shared<InvokerImpl>(), tipset.height);

    adt::Array<MessageReceipt> receipts;
    receipts.load(ipld);
    std::set<CID> processed_messages;
    for (auto &block : tipset.blks) {
      AwardBlockReward::Params reward{block.miner, 0, 0};
      auto apply_message =
          [&](const CID &cid,
              const UnsignedMessage &message) -> outcome::result<void> {
        if (!processed_messages.insert(cid).second) {
          return outcome::success();
        }

        OUTCOME_TRY(receipt, env->applyMessage(message));
        OUTCOME_TRY(receipts.append(std::move(receipt)));
        return outcome::success();
      };

      OUTCOME_TRY(meta, ipld->getCbor<MsgMeta>(block.messages));
      meta.load(ipld);
      OUTCOME_TRY(meta.bls_messages.visit(
          [&](auto, auto &cid) -> outcome::result<void> {
            OUTCOME_TRY(message, ipld->getCbor<UnsignedMessage>(cid));
            return apply_message(cid, message);
          }));
      OUTCOME_TRY(meta.secp_messages.visit(
          [&](auto, auto &cid) -> outcome::result<void> {
            OUTCOME_TRY(message, ipld->getCbor<SignedMessage>(cid));
            return apply_message(cid, message.message);
          }));
      OUTCOME_TRY(reward_encoded, codec::cbor::encode(reward));
      OUTCOME_TRY(env->applyImplicitMessage(UnsignedMessage{
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

    OUTCOME_TRY(receipts.flush());

    return Result{
        new_state_root,
        receipts.amt.cid(),
    };
  }

  bool InterpreterImpl::hasDuplicateMiners(
      const std::vector<BlockHeader> &blocks) const {
    std::set<Address> set;
    for (auto &block : blocks) {
      if (!set.insert(block.miner).second) {
        return true;
      }
    }
    return false;
  }
}  // namespace fc::vm::interpreter
