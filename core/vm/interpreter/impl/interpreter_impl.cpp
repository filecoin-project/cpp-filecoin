/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/interpreter/impl/interpreter_impl.hpp"

#include "blockchain/impl/weight_calculator_impl.hpp"
#include "const.hpp"
#include "primitives/tipset/load.hpp"
#include "vm/actor/builtin/v0/cron/cron_actor.hpp"
#include "vm/actor/builtin/v0/reward/reward_actor.hpp"
#include "vm/actor/impl/invoker_impl.hpp"
#include "vm/runtime/impl/runtime_impl.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

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
  using actor::Actor;
  using actor::InvokerImpl;
  using actor::kCronAddress;
  using actor::kRewardAddress;
  using actor::kSystemActorAddress;
  using actor::MethodParams;
  using actor::builtin::v0::cron::EpochTick;
  using actor::builtin::v0::reward::AwardBlockReward;
  using message::Address;
  using message::SignedMessage;
  using message::UnsignedMessage;
  using primitives::TokenAmount;
  using primitives::block::MsgMeta;
  using primitives::tipset::MessageVisitor;
  using runtime::Env;
  using runtime::MessageReceipt;

  InterpreterCache::Key::Key(const TipsetKey &tsk) : key{tsk.hash()} {}

  InterpreterCache::InterpreterCache(std::shared_ptr<PersistentBufferMap> kv)
      : kv{kv} {}

  boost::optional<outcome::result<Result>> InterpreterCache::tryGet(
      const Key &key) const {
    boost::optional<outcome::result<Result>> result;
    if (kv->contains(key.key)) {
      const auto raw{kv->get(key.key).value()};
      if (auto cached{
              codec::cbor::decode<boost::optional<Result>>(raw).value()}) {
        result.emplace(std::move(*cached));
      } else {
        result.emplace(InterpreterError::kTipsetMarkedBad);
      }
    }
    return result;
  }

  outcome::result<Result> InterpreterCache::get(const Key &key) const {
    if (auto cached{tryGet(key)}) {
      return *cached;
    }
    return InterpreterError::kNotCached;
  }

  void InterpreterCache::set(const Key &key, const Result &result) {
    kv->put(key.key, codec::cbor::encode(result).value()).value();
  }

  void InterpreterCache::markBad(const Key &key) {
    static const auto kNull{codec::cbor::encode(nullptr).value()};
    kv->put(key.key, kNull).value();
  }

  void InterpreterCache::remove(const Key &key) {
    kv->remove(key.key).value();
  }

  InterpreterImpl::InterpreterImpl(
      const Env0 &env0, std::shared_ptr<WeightCalculator> weight_calculator)
      : env0_{env0}, weight_calculator_{std::move(weight_calculator)} {}

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
    auto &ipld{env0_.ipld};

    auto on_receipt{[&](auto &receipt) {
      if (all_receipts) {
        all_receipts->push_back(receipt);
      }
    }};

    if (hasDuplicateMiners(tipset->blks)) {
      return InterpreterError::kDuplicateMiner;
    }

    auto env = std::make_shared<Env>(env0_, ts_branch, tipset);

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
      OUTCOME_TRY(parent, env0_.ts_load->load(tipset->getParents()));
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

    return Result{
        new_state_root,
        receipts.amt.cid(),
        std::move(weight),
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

  outcome::result<BigInt> InterpreterImpl::getWeight(
      const TipsetCPtr &tipset) const {
    if (weight_calculator_) {
      return weight_calculator_->calculateWeight(*tipset);
    }
    return 0;
  }

  CachedInterpreter::CachedInterpreter(std::shared_ptr<Interpreter> interpreter,
                                       std::shared_ptr<InterpreterCache> cache)
      : interpreter{std::move(interpreter)}, cache{std::move(cache)} {}

  outcome::result<Result> CachedInterpreter::interpret(
      TsBranchPtr ts_branch, const TipsetCPtr &tipset) const {
    InterpreterCache::Key key{tipset->key};
    if (auto cached{cache->tryGet(key)}) {
      return *cached;
    }
    auto result = interpreter->interpret(ts_branch, tipset);
    if (!result) {
      cache->markBad(key);
    } else {
      cache->set(key, result.value());
    }
    return result;
  }
}  // namespace fc::vm::interpreter
