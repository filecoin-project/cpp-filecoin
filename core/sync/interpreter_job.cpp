/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "interpreter_job.hpp"

#include "common/logger.hpp"
#include "vm/interpreter/impl/interpreter_impl.hpp"

namespace fc::sync {

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("interpreter");
      return logger.get();
    }
  }  // namespace

  InterpreterJob::InterpreterJob(
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

  outcome::result<void> InterpreterJob::start(const TipsetKey &head) {
    if (active_) {
      log()->warn("current job ({} -> {}) is still active, cancelling it",
                  status_.current_height,
                  status_.target_height);
      std::ignore = cancel();
    }

    OUTCOME_TRYA(target_head_, chain_db_.getTipsetByKey(head));
    status_.target_height = target_head_->height();

    const auto &hash = target_head_->key.hash();

    // maybe already interpreted
    OUTCOME_TRY(maybe_result,
                vm::interpreter::getSavedResult(*kv_store_, target_head_));
    if (maybe_result) {
      result_.last_interpreted = target_head_;
      result_.result = std::move(maybe_result.value());
      status_.current_height = status_.target_height;
      scheduleResult();
      return outcome::success();
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
        result_.last_interpreted = std::move(tipset);
        result_.result = std::move(res.value().value());
        return false;
      }
      return (!e);
    }));

    if (e) {
      return e;
    }

    if (!result_.last_interpreted) {
      // at least genesis tipset must be interpreted
      log()->error("cannot find highest interpreted tipset down from {}:{}",
                   target_head_->height(),
                   target_head_->key.toPrettyString());
      cancel();
      return vm::interpreter::InterpreterError::kChainInconsistency;
    }

    status_.current_height = result_.last_interpreted->height();
    log()->info(
        "starting {} -> {}", status_.current_height, status_.target_height);
    active_ = true;

    scheduleStep();
    return outcome::success();
  }

  InterpreterJob::Status InterpreterJob::cancel() {
    Status status = std::move(status_);
    active_ = false;
    status_.current_height = status_.target_height = 0;
    result_.last_interpreted.reset();
    result_.result = vm::interpreter::InterpreterError::kChainInconsistency;
    target_head_.reset();
    next_steps_.clear();
    step_cursor_ = 0;
    cb_handle_.cancel();
    return status;
  }

  const InterpreterJob::Status &InterpreterJob::getStatus() const {
    return status_;
  }

  void InterpreterJob::scheduleResult() {
    active_ = false;
    next_steps_.clear();
    step_cursor_ = 0;
    cb_handle_ = scheduler_.schedule([wptr{weak_from_this()}] {
      auto self = wptr.lock();
      if (self) {
        self->callback_(self->result_);
      }
    });
  }

  void InterpreterJob::scheduleStep() {
    if (!active_) {
      return;
    }
    cb_handle_ = scheduler_.schedule([wptr{weak_from_this()}] {
      auto self = wptr.lock();
      if (self) {
        self->nextStep();
      }
    });
  }

  void InterpreterJob::nextStep() {
    if (!active_) {
      return;
    }

    TipsetCPtr tipset = getNextTipset();
    if (!tipset) {
      return;
    }

    assert(result_.result);
    assert(tipset->getParents() == result_.last_interpreted->key);

    const auto &parent_res = result_.result.value();
    if (tipset->getParentStateRoot() != parent_res.state_root
        || tipset->getParentMessageReceipts() != parent_res.message_receipts) {
      log()->error("detected chain inconsistency at height {}",
                   tipset->height());
      // TODO (artem) maybe mark tipset as bad ???
      std::ignore = kv_store_->remove(common::Buffer(tipset->key.hash()));
      result_.result = vm::interpreter::InterpreterError::kChainInconsistency;
      scheduleResult();
      return;
    }

    status_.current_height = tipset->height();
    log()->info("doing {}/{}", status_.current_height, status_.target_height);
    result_.result = interpreter_->interpret(ipld_, tipset);

    if (!result_.result) {
      log()->error("stopped at height {} with error: {}",
                   status_.current_height,
                   result_.result.error().message());
      scheduleResult();
      return;
    }

    result_.last_interpreted = std::move(tipset);
    if (status_.current_height == status_.target_height) {
      log()->info("done");
      scheduleResult();
      return;
    }

    scheduleStep();
  }

  TipsetCPtr InterpreterJob::getNextTipset() {
    // step is cached
    if (step_cursor_ < next_steps_.size()) {
      return std::move(next_steps_[step_cursor_++]);
    }

    // clear cache
    next_steps_.clear();
    step_cursor_ = 0;

    assert(active_);
    assert(status_.target_height >= status_.current_height);

    size_t limit = status_.target_height - status_.current_height;
    if (limit == 0) {
      // done, though should not get here
      scheduleResult();
      return nullptr;
    }

    // dont walk forward too far, it takes time
    static constexpr size_t kQueryLimit = 100;
    if (limit > kQueryLimit) {
      limit = kQueryLimit;
    }

    TipsetCPtr next;

    auto res =
        chain_db_.walkForward(result_.last_interpreted,
                              target_head_,
                              limit,
                              [&, this](TipsetCPtr tipset) {
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

    scheduleResult();
    return nullptr;
  }

}  // namespace fc::sync
