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
}  // namespace fc
