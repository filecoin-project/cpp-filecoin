/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>
#include <memory>
#include <vector>

namespace fc {
  template <typename T>
  std::weak_ptr<T> weaken(const std::shared_ptr<T> &ptr) {
    return ptr;
  }

  template <typename T, typename F>
  auto weakCb0(std::weak_ptr<T> weak, F &&f) {
    return [weak, f{std::forward<F>(f)}](auto &&...args) {
      if (auto ptr{weak.lock()}) {
        f(std::forward<decltype(args)>(args)...);
      }
    };
  }

  template <typename T, typename F>
  void weakFor(std::vector<std::weak_ptr<T>> &ws, const F &f) {
    ws.erase(std::remove_if(ws.begin(),
                            ws.end(),
                            [&](auto &w) {
                              if (auto s{w.lock()}) {
                                f(s);
                                return false;
                              }
                              return true;
                            }),
             ws.end());
  }

  template <typename L, typename R>
  bool ownerEq(const L &l, const R &r) {
    return !l.owner_before(r) && !r.owner_before(l);
  }

  // https://stackoverflow.com/a/45507610
  template <typename T>
  inline bool weakEmpty(const std::weak_ptr<T> &weak) {
    const static std::weak_ptr<T> empty;
    return ownerEq(weak, empty);
  }
}  // namespace fc
