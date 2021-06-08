/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/interpreter/impl/interpreter_impl.hpp"

#include "const.hpp"
#include "primitives/tipset/load.hpp"
#include "vm/actor/builtin/v0/cron/cron_actor.hpp"
#include "vm/actor/builtin/v0/reward/reward_actor.hpp"
#include "vm/runtime/env.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::vm::interpreter, InterpreterError, e) {
  using E = fc::vm::interpreter::InterpreterError;
  switch (e) {
    case E::kDuplicateMiner:
      return "InterpreterError: Duplicate miner";
    case E::kTipsetMarkedBad:
      return "InterpreterError: Tipset marked as bad";
    case E::kChainInconsistency:
      return "InterpreterError: chain inconsistency";
    case E::kNotCached:
      return "InterpreterError::kNotCached";
    default:
      break;
  }
  return "InterpreterError: unknown code";
}

namespace fc::vm::interpreter {
  using actor::kCronAddress;
  using actor::kRewardAddress;
  using actor::kSystemActorAddress;
  using actor::MethodParams;
  using actor::builtin::v0::cron::EpochTick;
  using actor::builtin::v0::reward::AwardBlockReward;
  using message::UnsignedMessage;
  using primitives::address::Address;
  using primitives::tipset::MessageVisitor;
  using runtime::Env;
  using runtime::MessageReceipt;

  InterpreterImpl::InterpreterImpl(
      const EnvironmentContext &env_context,
      std::shared_ptr<WeightCalculator> weight_calculator)
      : env_context_{env_context},
        weight_calculator_{std::move(weight_calculator)} {}

  outcome::result<Result> InterpreterImpl::interpret(
      TsBranchPtr ts_branch, const TipsetCPtr &tipset) const {
    if (tipset->height() == 0) {
      OUTCOME_TRY(weight, getWeight(tipset));
      return Result{
          tipset->getParentStateRoot(),
          tipset->getParentMessageReceipts(),
          weight,
      };
    }
    return applyBlocks(ts_branch, tipset, {});
  }

  outcome::result<Result> InterpreterImpl::applyBlocks(
      TsBranchPtr ts_branch,
      const TipsetCPtr &tipset,
      std::vector<MessageReceipt> *all_receipts) const {
    auto &ipld{env_context_.ipld};

    auto on_receipt{[&](auto &receipt) {
      if (all_receipts) {
        all_receipts->push_back(receipt);
      }
    }};

    if (hasDuplicateMiners(tipset->blks)) {
      return InterpreterError::kDuplicateMiner;
    }

    auto env = std::make_shared<Env>(env_context_, ts_branch, tipset);

    auto cron{[&]() -> outcome::result<void> {
      OUTCOME_TRY(receipt,
                  env->applyImplicitMessage(UnsignedMessage{
                      kCronAddress,
                      kSystemActorAddress,
                      env->epoch,
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

    if (tipset->height() > 1) {
      OUTCOME_TRY(parent, env_context_.ts_load->load(tipset->getParents()));
      for (auto epoch{parent->height() + 1}; epoch < tipset->height();
           ++epoch) {
        env->epoch = epoch;
        OUTCOME_TRY(cron());
      }
      env->epoch = tipset->height();
    }

    adt::Array<MessageReceipt> receipts{ipld};
    MessageVisitor message_visitor{ipld, true, true};
    for (auto &block : tipset->blks) {
      AwardBlockReward::Params reward{
          block.miner, 0, 0, block.election_proof.win_count};
      OUTCOME_TRY(message_visitor.visit(
          block,
          [&](auto, auto bls, auto &cid, auto, auto *msg)
              -> outcome::result<void> {
            OUTCOME_TRY(raw, ipld->get(cid));
            OUTCOME_TRY(apply, env->applyMessage(*msg, raw.size()));
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
                      tipset->height(),
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
    OUTCOME_TRY(env->ipld->flush(new_state_root));

    OUTCOME_TRY(Ipld::flush(receipts));

    OUTCOME_TRY(weight, getWeight(tipset));

    return Result{new_state_root, receipts.amt.cid(), std::move(weight)};
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

  outcome::result<BigInt> InterpreterImpl::getWeight(
      const TipsetCPtr &tipset) const {
    if (weight_calculator_) {
      return weight_calculator_->calculateWeight(*tipset);
    }
    return 0;
  }
}  // namespace fc::vm::interpreter
