/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

namespace fc {
  template <typename T, typename F>
  auto weakCb0(std::weak_ptr<T> weak, F &&f) {
    return [weak, f{std::forward<F>(f)}](auto &&... args) {
      if (auto ptr{weak.lock()}) {
        f(std::forward<decltype(args)>(args)...);
      }
    };
  }
}  // namespace fc
