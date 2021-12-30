/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace ledger {

  using Byte = uint8_t;
  using Bytes = std::vector<Byte>;
  using Error = std::optional<std::string>;

  // RAII wrapper
  class Defer {
   public:
    Defer(std::function<void()> cb) : cb(std::move(cb)) {}

    Defer(const Defer &) = delete;
    Defer &operator=(const Defer &) = delete;
    Defer(Defer &&) = delete;
    Defer &operator=(Defer &&) = delete;

    ~Defer() {
      if (cb) {
        cb();
      }
    }

   private:
    std::function<void()> cb;
  };

}  // namespace ledger
