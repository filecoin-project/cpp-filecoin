/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "interpret_job.hpp"

#include <libp2p/protocol/common/scheduler.hpp>

#include "blockchain/weight_calculator.hpp"
#include "chain_db.hpp"
#include "common/logger.hpp"
#include "events.hpp"
#include "vm/interpreter/interpreter.hpp"

namespace fc::sync {

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("interpret_job");
      return logger.get();
    }

    constexpr auto kDefaultCurrentResult =
        vm::interpreter::InterpreterError::kChainInconsistency;

  }  // namespace

  class InterpretJobImpl : public InterpretJob {
   public:
    InterpretJobImpl(
        std::shared_ptr<storage::PersistentBufferMap> kv_store,
        std::shared_ptr<vm::interpreter::Interpreter> interpreter,
        std::shared_ptr<libp2p::protocol::Scheduler> scheduler,
        std::shared_ptr<ChainDb> chain_db,
        IpldPtr ipld,
        std::shared_ptr<blockchain::weight::WeightCalculator> weight_calculator)
        : kv_store_(kv_store),
          interpreter_(std::move(interpreter)),
          scheduler_(std::move(scheduler)),
          chain_db_(std::move(chain_db)),
          ipld_(std::move(ipld)),
          weight_calculator_(std::move(weight_calculator)),
          current_result_(kDefaultCurrentResult) {
      assert(kv_store_);
      assert(interpreter_);
      assert(scheduler_);
      assert(chain_db_);
      assert(ipld_);
      assert(weight_calculator_);
    }

   private:
    void start(std::shared_ptr<events::Events> events) override {
      events_ = std::move(events);
      assert(events_);

      head_downloaded_event_ = events_->subscribeHeadDownloaded(
          [this](const events::HeadDownloaded &e) {
            newJob(std::move(e.head));
          });
    }

    bool isActive() {
      bool active = (target_head_ != nullptr);
      if (active) {
        assert(targetHeight() > currentHeight());
        assert(current_result_.has_value());
      }
      return active;
    }

    Height targetHeight() {
      assert(target_head_);
      return target_head_->height();
    }

    Height currentHeight() {
      assert(current_head_);
      return current_head_->height();
    }

    void clearState() {
      current_head_.reset();
      current_result_ = kDefaultCurrentResult;
      target_head_.reset();
      next_steps_.clear();
      step_cursor_ = 0;
      cb_handle_.cancel();
    }

    void dequeueNewJob() {
      assert(!isActive());
      if (!pending_targets_.empty()) {
        log()->debug("dequeueing next job (total count: {})",
                     pending_targets_.size());
        auto it = pending_targets_.begin();
        auto head = it->second;
        pending_targets_.erase(it);
        newJob(std::move(head));
      }
    }

    void done() {
      BigInt weight;
      if (current_result_.has_value()) {
        assert(target_head_ == current_head_);
        auto weight_res = weight_calculator_->calculateWeight(*target_head_);
        if (!weight_res) {
          // practically impossible if it was indeed interpreted
          current_result_ = weight_res.error();
        } else {
          weight = weight_res.value();
        }
      }

      log()->debug("done ({})",
                   current_result_.has_value()
                       ? "OK"
                       : current_result_.error().message());

      events_->signalHeadInterpreted({.head = std::move(target_head_),
                                      .result = std::move(current_result_),
                                      .weight = std::move(weight)});

      clearState();
      dequeueNewJob();
    }

    // returns true to proceed with this candidate in async manner
    bool proceedWithTarget(TipsetCPtr head) {
      assert(head);
      assert(events_);

      if (!chain_db_->isHead(head->key.hash())) {
        // we reached this point, but heads moved
        log()->debug("target is no more a head (h={}), ignoring it",
                     head->height());
        return false;
      }

      events::HeadInterpreted event{
          .result = vm::interpreter::InterpreterError::kChainInconsistency};

      bool proceed = false;

      // maybe this head already interpreted
      auto maybe_result =
          vm::interpreter::getSavedResult(*kv_store_, target_head_);
      if (!maybe_result) {
        // bad tipset or internal error
        event.result = maybe_result.error();
      } else if (maybe_result.value().has_value()) {
        // interpreted successfully and ready to become a head
        auto weight = weight_calculator_->calculateWeight(*head);
        if (!weight) {
          // practically impossible if it was indeed interpreted
          event.result = weight.error();
        } else {
          // head is ready!
          event.result = std::move(maybe_result.value().value());
          event.weight = weight.value();
        }
      } else if (head->height() == 0) {
        // genesis is not yet interpreted i.e. we are running node 1st time
        bool genesis_ok = false;
        event.result = interpreter_->interpret(ipld_, target_head_);
        if (event.result) {
          auto weight = weight_calculator_->calculateWeight(*head);
          if (!weight) {
            event.result = weight.error();
          } else {
            event.weight = weight.value();
            genesis_ok = true;
          }
        }
        if (!genesis_ok) {
          // cannot proceed with this genesis
          events_->signalFatalError({.message = "Cannot interpret genesis"});
          log()->critical("cannot interpret genesis, {}",
                          event.result.error().message());
        }
      } else {
        // will do in async manner
        proceed = true;
      }

      if (!proceed) {
        event.head = std::move(head);
        events_->signalHeadInterpreted(std::move(event));
      }

      return proceed;
    }

    void newJob(TipsetCPtr head) {
      if (!proceedWithTarget(head)) {
        return;
      }

      if (isActive()) {
        if (*target_head_ == *head) {
          return;
        }

        auto [_, inserted] = pending_targets_.insert({head->height(), head});

        if (inserted) {
          log()->debug(
              "current job ({} -> {}) is still active, moving new job (h={}} "
              "to "
              "pending targets",
              currentHeight(),
              targetHeight(),
              head->height());
        } else {
          log()->debug("ignoring already scheduled target of height {}",
                       head->height());
        }

        return;
      }

      target_head_ = std::move(head);
      const auto &hash = target_head_->key.hash();

      std::error_code e;

      // find the highest interpreted tipset in chain backwards from the target
      auto res = chain_db_->walkBackward(hash, 0, [&, this](TipsetCPtr tipset) {
        if (e) {
          return false;
        }
        auto res = vm::interpreter::getSavedResult(*kv_store_, tipset);
        if (!res) {
          log()->error("something wrong at height {}, {}",
                       tipset->height(),
                       res.error().message());
          e = res.error();
        } else if (res.value().has_value()) {
          current_head_ = std::move(tipset);
          current_result_ = std::move(res.value().value());
          return false;
        }
        return (!e);
      });

      if (!res) {
        e = res.error();
      }

      if (e) {
        events_->signalHeadInterpreted(
            {.head = std::move(head), .result = e, .weight = 0});
        clearState();
        return;
      }

      if (!current_head_) {
        // at least genesis tipset must be interpreted
        log()->error("cannot find highest interpreted tipset down from h={}",
                     head->height());
        events_->signalHeadInterpreted(
            {.head = std::move(head),
             .result = vm::interpreter::InterpreterError::kChainInconsistency,
             .weight = 0});
        events_->signalFatalError(
            {.message = "Chain inconsistency, head is not a head"});
        clearState();
        return;
      }

      target_head_ = std::move(head);

      log()->info("starting new job {} -> {}", currentHeight(), targetHeight());

      scheduleStep();
    }

    void scheduleStep() {
      if (!isActive()) {
        return;
      }

      // TODO (artem) fix there operator=
      cb_handle_.cancel();
      cb_handle_ = scheduler_->schedule([this] { nextStep(); });
    }

    void nextStep() {
      if (!isActive()) {
        return;
      }

      TipsetCPtr tipset = getNextTipset();
      if (!tipset) {
        done();
        return;
      }

      assert(current_result_.has_value());
      assert(tipset->getParents() == current_head_->key);

      const auto &parent_res = current_result_.value();
      if (tipset->getParentStateRoot() != parent_res.state_root
          || tipset->getParentMessageReceipts()
                 != parent_res.message_receipts) {
        log()->error("detected chain inconsistency at height {}",
                     tipset->height());

        // TODO (artem) maybe mark tipset as bad ???
        std::ignore = kv_store_->remove(common::Buffer(tipset->key.hash()));

        current_result_ =
            vm::interpreter::InterpreterError::kChainInconsistency;

        done();
        return;
      }

      current_head_ = std::move(tipset);

      log()->info("doing {}/{}", currentHeight(), targetHeight());

      current_result_ = interpreter_->interpret(ipld_, current_head_);

      if (!current_result_) {
        log()->error("stopped at height {}, {}",
                     currentHeight(),
                     current_result_.error().message());
        done();
        return;
      }

      current_head_ = std::move(tipset);
      if (currentHeight() == targetHeight()) {
        done();
      } else {
        scheduleStep();
      }
    }

    TipsetCPtr getNextTipset() {
      // step is cached
      if (step_cursor_ < next_steps_.size()) {
        return std::move(next_steps_[step_cursor_++]);
      }

      // clear cache
      next_steps_.clear();
      step_cursor_ = 0;

      assert(isActive());

      auto target_height = targetHeight();

      size_t limit = target_height - currentHeight();
      if (limit == 0) {
        // done, though should not get here
        return nullptr;
      }

      // dont walk forward too far, it takes time
      static constexpr size_t kQueryLimit = 100;
      if (limit > kQueryLimit) {
        limit = kQueryLimit;
      }

      TipsetCPtr next;

      auto res = chain_db_->walkForward(
          current_head_, target_head_, limit, [&, this](TipsetCPtr tipset) {
            if (tipset->height() > target_height) {
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
                     currentHeight() + 1);
        current_result_ = res.error();
        next_steps_.clear();
        next.reset();
      }

      if (next) {
        log()->debug("scheduled {} tipsets starting from height {}",
                     next_steps_.size() + 1,
                     currentHeight() + 1);
      }

      return next;
    }

    std::shared_ptr<storage::PersistentBufferMap> kv_store_;
    std::shared_ptr<vm::interpreter::Interpreter> interpreter_;
    std::shared_ptr<libp2p::protocol::Scheduler> scheduler_;
    std::shared_ptr<ChainDb> chain_db_;
    IpldPtr ipld_;
    std::shared_ptr<blockchain::weight::WeightCalculator> weight_calculator_;
    std::shared_ptr<events::Events> events_;

    /// Scheduled jobs
    std::set<std::pair<Height, TipsetCPtr>> pending_targets_;

    /// Current job state
    TipsetCPtr current_head_;
    outcome::result<vm::interpreter::Result> current_result_;
    TipsetCPtr target_head_;

    /// Next steps cache
    std::vector<TipsetCPtr> next_steps_;
    size_t step_cursor_ = 0;

    events::Connection head_downloaded_event_;
    libp2p::protocol::scheduler::Handle cb_handle_;
  };

  std::shared_ptr<InterpretJob> InterpretJob::create(
      std::shared_ptr<storage::PersistentBufferMap> kv_store,
      std::shared_ptr<vm::interpreter::Interpreter> interpreter,
      std::shared_ptr<libp2p::protocol::Scheduler> scheduler,
      std::shared_ptr<ChainDb> chain_db,
      IpldPtr ipld,
      std::shared_ptr<blockchain::weight::WeightCalculator> weight_calculator) {
    return std::make_shared<InterpretJobImpl>(std::move(kv_store),
                                              std::move(interpreter),
                                              std::move(scheduler),
                                              std::move(chain_db),
                                              std::move(ipld),
                                              std::move(weight_calculator)

    );
  }

}  // namespace fc::sync
