/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/interpreter/impl/interpreter_impl.hpp"

#include <utility>

#include "blockchain/block_validator/validator.hpp"
#include "common/prometheus/metrics.hpp"
#include "common/prometheus/since.hpp"
#include "const.hpp"
#include "primitives/tipset/load.hpp"
#include "vm/actor/builtin/methods/cron.hpp"
#include "vm/actor/builtin/methods/reward.hpp"
#include "vm/runtime/make_vm.hpp"
#include "vm/toolchain/toolchain.hpp"

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
  using message::UnsignedMessage;
  using primitives::address::Address;
  using primitives::tipset::MessageVisitor;
  using runtime::MessageReceipt;
  namespace cron = actor::builtin::cron;
  namespace reward = actor::builtin::reward;

  InterpreterImpl::InterpreterImpl(
      EnvironmentContext env_context,
      std::shared_ptr<BlockValidator> validator,
      std::shared_ptr<WeightCalculator> weight_calculator)
      : env_context_{std::move(env_context)},
        validator_{std::move(validator)},
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

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  outcome::result<Result> InterpreterImpl::applyBlocks(
      TsBranchPtr ts_branch,
      const TipsetCPtr &tipset,
      std::vector<MessageReceipt> *all_receipts) const {
    const auto &ipld{env_context_.ipld};

    if (validator_) {
      for (const auto &block : tipset->blks) {
        OUTCOME_TRY(validator_->validate(ts_branch, block));
      }
    }

    static auto &metricFailure{
        prometheus::BuildCounter()
            .Name("lotus_block_failure")
            .Help("Counter for block validation failures")
            .Register(prometheusRegistry())
            .Add({})};
    static auto &metricSuccess{
        prometheus::BuildCounter()
            .Name("lotus_block_success")
            .Help("Counter for block validation successes")
            .Register(prometheusRegistry())
            .Add({})};
    static auto &metricTotal{prometheus::BuildHistogram()
                                 .Name("lotus_vm_applyblocks_total_ms")
                                 .Help("Time spent applying block state")
                                 .Register(prometheusRegistry())
                                 .Add({}, kDefaultPrometheusMsBuckets)};
    static auto &metricMessages{prometheus::BuildHistogram()
                                    .Name("lotus_vm_applyblocks_messages")
                                    .Help("Time spent applying block messages")
                                    .Register(prometheusRegistry())
                                    .Add({}, kDefaultPrometheusMsBuckets)};
    static auto &metricEarly{
        prometheus::BuildHistogram()
            .Name("lotus_vm_applyblocks_early")
            .Help("Time spent in early apply-blocks (null cron, upgrades)")
            .Register(prometheusRegistry())
            .Add({}, kDefaultPrometheusMsBuckets)};
    static auto &metricCron{prometheus::BuildHistogram()
                                .Name("lotus_vm_applyblocks_cron")
                                .Help("Time spent in cron")
                                .Register(prometheusRegistry())
                                .Add({}, kDefaultPrometheusMsBuckets)};
    static auto &metricFlush{prometheus::BuildHistogram()
                                 .Name("lotus_vm_applyblocks_flush")
                                 .Help("Time spent flushing vm state")
                                 .Register(prometheusRegistry())
                                 .Add({}, kDefaultPrometheusMsBuckets)};

    bool success{false};
    const Since since;
    std::pair<::prometheus::Histogram *, Since> last_step;
    auto nextStep{[&](auto metric) {
      if (last_step.first) {
        last_step.first->Observe(last_step.second.ms());
      }
      last_step = std::make_pair(metric, Since{});
    }};
    auto BOOST_OUTCOME_TRY_UNIQUE_NAME{gsl::finally([&] {
      metricTotal.Observe(since.ms());
      nextStep(nullptr);
      (success ? metricSuccess : metricFailure).Increment();
    })};
    nextStep(&metricEarly);

    auto on_receipt{[&](auto &receipt) {
      if (all_receipts) {
        all_receipts->push_back(receipt);
      }
    }};

    if (hasDuplicateMiners(tipset->blks)) {
      return InterpreterError::kDuplicateMiner;
    }

    const auto buf_ipld{std::make_shared<IpldBuffered>(ipld)};
    auto state{tipset->getParentStateRoot()};
    auto epoch{tipset->epoch()};
    std::shared_ptr<VirtualMachine> env;

    auto cron{[&]() -> outcome::result<void> {
      OUTCOME_TRY(receipt,
                  env->applyImplicitMessage(UnsignedMessage{
                      kCronAddress,
                      kSystemActorAddress,
                      static_cast<uint64_t>(epoch),
                      0,
                      0,
                      kBlockGasLimit * 10000,
                      cron::EpochTick::Number,
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
      for (epoch = parent->height() + 1; epoch < tipset->height(); ++epoch) {
        OUTCOME_TRYA(env,
                     makeVm(buf_ipld,
                            env_context_,
                            ts_branch,
                            tipset->getParentBaseFee(),
                            state,
                            epoch));
        OUTCOME_TRY(cron());
        OUTCOME_TRYA(state, env->flush());
      }
      epoch = tipset->height();
    }
    OUTCOME_TRYA(env,
                 makeVm(buf_ipld,
                        env_context_,
                        ts_branch,
                        tipset->getParentBaseFee(),
                        state,
                        epoch));

    nextStep(&metricMessages);

    adt::Array<MessageReceipt> receipts{ipld};
    MessageVisitor message_visitor{ipld, true, true};
    for (const auto &block : tipset->blks) {
      reward::AwardBlockReward::Params reward{
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
                      static_cast<uint64_t>(tipset->height()),
                      0,
                      0,
                      1 << 30,
                      reward::AwardBlockReward::Number,
                      MethodParams{reward_encoded},
                  }));
      if (receipt.exit_code != VMExitCode::kOk) {
        return receipt.exit_code;
      }
      on_receipt(receipt);
    }

    nextStep(&metricCron);

    OUTCOME_TRY(cron());

    nextStep(&metricFlush);

    OUTCOME_TRY(new_state_root, env->flush());
    OUTCOME_TRY(buf_ipld->flush(new_state_root));

    OUTCOME_TRY(receipts.amt.flush());

    OUTCOME_TRY(weight, getWeight(tipset));

    success = true;

    return Result{new_state_root, receipts.amt.cid(), std::move(weight)};
  }

  bool InterpreterImpl::hasDuplicateMiners(
      const std::vector<BlockHeader> &blocks) {
    std::set<Address> set;
    for (const auto &block : blocks) {
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
