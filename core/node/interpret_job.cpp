/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "interpret_job.hpp"

#include "common/logger.hpp"
#include "vm/interpreter/impl/interpreter_impl.hpp"

namespace fc::sync {

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("interpret_job");
      return logger.get();
    }
  }  // namespace

  InterpretJob::InterpretJob(
      std::shared_ptr<storage::PersistentBufferMap> kv_store,
      std::shared_ptr<vm::interpreter::Interpreter> interpreter,
      libp2p::protocol::Scheduler &scheduler,
      ChainDb &chain_db,
      IpfsStoragePtr ipld,
      Callback callback)
      : kv_store_(kv_store),
        interpreter_(std::move(interpreter)),
        scheduler_(scheduler),
        chain_db_(chain_db),
        ipld_(std::move(ipld)),
        callback_(std::move(callback)),
        result_{nullptr,
                vm::interpreter::InterpreterError::kChainInconsistency} {
    assert(callback_);
  }

  outcome::result<boost::optional<InterpretJob::Result>> InterpretJob::start(
      const TipsetKey &head) {
    if (status_.active) {
      log()->warn("current job ({} -> {}) is still active, cancelling it",
                  status_.current_height,
                  status_.target_height);
      std::ignore = cancel();
    }

    OUTCOME_TRYA(target_head_, chain_db_.getTipsetByKey(head));

    const auto &hash = target_head_->key.hash();

    // maybe already interpreted
    OUTCOME_TRY(maybe_result,
                vm::interpreter::getSavedResult(*kv_store_, target_head_));
    if (maybe_result) {
      return Result{.head = target_head_,
                    .result = std::move(maybe_result.value())};
    }

    if (target_head_->height() == 0) {
      // genesis not yet interpreted
      return Result{.head = target_head_,
          .result = interpreter_->interpret(ipld_, target_head_)};
    }

    std::error_code e;

    // find the highest interpreted tipset in chain backwards from the target
    OUTCOME_TRY(chain_db_.walkBackward(hash, 0, [&, this](TipsetCPtr tipset) {
      if (e) {
        return false;
      }
      auto res = vm::interpreter::getSavedResult(*kv_store_, tipset);
      if (!res) {
        e = res.error();
      } else if (res.value()) {
        result_.head = std::move(tipset);
        result_.result = std::move(res.value().value());
        return false;
      }
      return (!e);
    }));

    if (e) {
      return e;
    }

    if (!result_.head) {
      // at least genesis tipset must be interpreted
      log()->error("cannot find highest interpreted tipset down from {}:{}",
                   target_head_->height(),
                   target_head_->key.toPrettyString());
      cancel();
      return vm::interpreter::InterpreterError::kChainInconsistency;
    }

    status_.target_height = target_head_->height();
    status_.current_height = result_.head->height();
    log()->info(
        "starting {} -> {}", status_.current_height, status_.target_height);
    status_.active = true;

    scheduleStep();
    return boost::none;
  }

  InterpretJob::Status InterpretJob::cancel() {
    Status status = std::move(status_);
    status_.active = false;
    status_.current_height = status_.target_height = 0;
    result_.head.reset();
    result_.result = vm::interpreter::InterpreterError::kChainInconsistency;
    target_head_.reset();
    next_steps_.clear();
    step_cursor_ = 0;
    return status;
  }

  const InterpretJob::Status &InterpretJob::getStatus() const {
    return status_;
  }

  void InterpretJob::done() {
    log()->debug(
        "done ({})",
        result_.result.has_value() ? "OK" : result_.result.error().message());
    status_.active = false;
    status_.current_height = status_.target_height = 0;
    next_steps_.clear();
    step_cursor_ = 0;
    callback_(std::move(result_));
  }

  void InterpretJob::scheduleStep() {
    if (!status_.active) {
      return;
    }
    cb_handle_ = scheduler_.schedule([this] {
      nextStep();
    });
  }

  void InterpretJob::nextStep() {
    if (!status_.active) {
      return;
    }

    TipsetCPtr tipset = getNextTipset();
    if (!tipset) {
      return;
    }

    assert(result_.result);
    assert(tipset->getParents() == result_.head->key);

    const auto &parent_res = result_.result.value();
    if (tipset->getParentStateRoot() != parent_res.state_root
        || tipset->getParentMessageReceipts() != parent_res.message_receipts) {
      log()->error("detected chain inconsistency at height {}",
                   tipset->height());
      // TODO (artem) maybe mark tipset as bad ???
      std::ignore = kv_store_->remove(common::Buffer(tipset->key.hash()));
      result_.result = vm::interpreter::InterpreterError::kChainInconsistency;
      done();
      return;
    }

    status_.current_height = tipset->height();
    log()->info("doing {}/{}", status_.current_height, status_.target_height);
    result_.result = interpreter_->interpret(ipld_, tipset);

    if (!result_.result) {
      log()->error("stopped at height {}",
                   status_.current_height);
      done();
      return;
    }

    result_.head = std::move(tipset);
    if (status_.current_height == status_.target_height) {
      done();
      return;
    }

    scheduleStep();
  }

  TipsetCPtr InterpretJob::getNextTipset() {
    // step is cached
    if (step_cursor_ < next_steps_.size()) {
      return std::move(next_steps_[step_cursor_++]);
    }

    // clear cache
    next_steps_.clear();
    step_cursor_ = 0;

    assert(status_.active);
    assert(status_.target_height >= status_.current_height);

    size_t limit = status_.target_height - status_.current_height;
    if (limit == 0) {
      // done, though should not get here
      done();
      return nullptr;
    }

    // dont walk forward too far, it takes time
    static constexpr size_t kQueryLimit = 100;
    if (limit > kQueryLimit) {
      limit = kQueryLimit;
    }

    TipsetCPtr next;

    auto res = chain_db_.walkForward(
        result_.head, target_head_, limit, [&, this](TipsetCPtr tipset) {
          if (tipset->height() > status_.target_height) {
            log()->error("walks behind height limit");
            return false;
          }
          if (next) {
            next_steps_.push_back(std::move(tipset));
          } else {
            next = std::move(tipset);
          }
          return true;
        });
    if (!res) {
      log()->error("failed to load {} tipsets starting from height {}",
                   limit,
                   status_.current_height + 1);
      result_.result = res.error();
      next_steps_.clear();
      next.reset();
    }

    if (next) {
      log()->debug("scheduled {} tipsets starting from height {}",
                   next_steps_.size() + 1,
                   status_.current_height + 1);
      return next;
    }

    done();
    return nullptr;
  }

}  // namespace fc::sync
