/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <functional>
#include <memory>

#include "common/outcome.hpp"

namespace fc {
  template <typename T>
  using CbT = std::function<void(outcome::result<T>)>;

  template <typename T>
  struct AsyncAll : std::enable_shared_from_this<AsyncAll<T>> {
    using Cb = CbT<std::vector<T>>;

    AsyncAll(int n, Cb cb) : n{n}, cb{std::move(cb)} {
      assert(n != 0);
      values.resize(n);
    }

    auto on(size_t i) {
      return [s{this->shared_from_this()}, i](auto _r) {
        if (!_r) {
          if (s->n.exchange(-1) > 0) {
            s->cb(_r.error());
          }
        } else {
          s->values[i] = std::move(_r.value());
          if (--s->n == 0) {
            s->cb(std::move(s->values));
          }
        }
      };
    }

    std::atomic<int> n;
    std::vector<T> values;
    Cb cb;
  };

  /**
   * Asynchronously waits for `N` call results and calls `final_callback` with
   * all results when all callback returned by 'on()' are executed.
   * @tparam CallContext - additional data to save for each callback call
   * @tparam Result - result of each callback
   */
  template <typename CallContext, typename Result>
  struct AsyncWaiter
      : std::enable_shared_from_this<AsyncWaiter<CallContext, Result>> {
    using FinalizeCallback =
        std::function<void(std::vector<std::pair<CallContext, Result>>)>;

    /**
     * Consrtucts waiter
     * @param expected_calls - expected calls before final_call is called
     * @param final_callback - final callback that is called with all callback
     * results
     */
    explicit AsyncWaiter(size_t expected_calls, FinalizeCallback final_callback)
        : final_callback_{std::move(final_callback)},
          expected_{expected_calls} {}

    /**
     * Returns callback to store result and capture context
     * @param call_context - additional data for callback call
     * @return one of callbacks to wait for
     */
    auto on(const CallContext &call_context) {
      return [self{this->shared_from_this()}, call_context](auto result) {
        std::lock_guard lock{self->mutex_};
        self->calls_.emplace_back(std::make_pair(call_context, result));
        if (--self->expected_ == 0) {
          self->final_callback_(self->calls_);
        }
      };
    }

   private:
    FinalizeCallback final_callback_;
    std::mutex mutex_;
    std::vector<std::pair<CallContext, Result>> calls_;
    size_t expected_;
  };
}  // namespace fc
