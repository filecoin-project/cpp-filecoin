/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include <libp2p/basic/scheduler.hpp>

namespace libp2p::basic {

  /**
   * TODO This code is duplicated from Libp2p
   * "libp2p/test/testutil/manual_scheduler_backend.hpp". Remove it as soon as
   * it is exported in libp2p
   */

  /**
   * Manual scheduler backend implementation, uses manual time shifts and
   * internal pseudo timer. Injected into SchedulerImpl.
   */
  class ManualSchedulerBackend : public SchedulerBackend {
    using Callback = std::function<void()>;

   public:
    ManualSchedulerBackend() : current_clock_() {}

    /**
     * @return Milliseconds since clock's epoch. Clock is set manually
     */
    std::chrono::milliseconds now() noexcept override {
      return current_clock_;
    }

    /**
     * Sets the timer. Called by Scheduler implementation
     * @param abs_time Abs time: milliseconds since clock's epoch
     * @param scheduler Weak ref to owner
     */
    void setTimer(std::chrono::milliseconds abs_time,
                  std::weak_ptr<SchedulerBackendFeedback> scheduler) override {
      if (abs_time == std::chrono::milliseconds::zero()) {
        deferred_callbacks_.emplace_back([scheduler = std::move(scheduler)]() {
          auto sch = scheduler.lock();
          if (sch) {
            sch->pulse(kZeroTime);
          }
        });
        return;
      }

      assert(abs_time.count() > 0);

      timer_expires_ = abs_time;

      timer_callback_ = [this, scheduler = std::move(scheduler)]() {
        auto sch = scheduler.lock();
        if (sch) {
          sch->pulse(now());
        }
      };
    }

    /**
     * Shifts internal timer by delta milliseconds, executes everything (i.e.
     * deferred and timed events) in between
     * @param delta Manual time shift in milliseconds
     */
    void shift(std::chrono::milliseconds delta) {
      if (delta.count() < 0) {
        delta = std::chrono::milliseconds::zero();
      }
      current_clock_ += delta;

      if (!deferred_callbacks_.empty()) {
        in_process_.swap(deferred_callbacks_);
        deferred_callbacks_.clear();
        for (const auto &cb : in_process_) {
          cb();
        }
        in_process_.clear();
      }

      if (timer_callback_ && current_clock_ >= timer_expires_) {
        Callback cb;
        cb.swap(timer_callback_);
        timer_expires_ = std::chrono::milliseconds::zero();
        cb();
      }
    }

    /**
     * Shifts internal timer to the nearest timer event, executes everything
     * (i.e. deferred and timed events) in between
     */
    void shiftToTimer() {
      auto delta = std::chrono::milliseconds::zero();
      if (timer_expires_ > current_clock_) {
        delta = timer_expires_ - current_clock_;
      }
      shift(delta);
    }

    /**
     * @return true if no more events scheduled
     */
    bool empty() const {
      return deferred_callbacks_.empty() && !timer_callback_;
    }

   private:
    /// Current time, set manually
    std::chrono::milliseconds current_clock_;

    /// Callbacks deferred for the next cycle
    std::vector<Callback> deferred_callbacks_;

    /// Currently processed callback (reentrancy + rescheduling reasons here)
    std::vector<Callback> in_process_;

    /// Timer callback
    Callback timer_callback_;

    /// Expiry of timer event
    std::chrono::milliseconds timer_expires_{};
  };

}  // namespace libp2p::basic
