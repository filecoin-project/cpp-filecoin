/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>

namespace fc {
  struct Since {
    using Clock = std::chrono::steady_clock;

    Clock::time_point start{Clock::now()};

    template <typename T = double>
    T ms() const {
      return std::chrono::duration_cast<std::chrono::duration<T, std::milli>>(
                 Clock::now() - start)
          .count();
    }
  };
}  // namespace fc
