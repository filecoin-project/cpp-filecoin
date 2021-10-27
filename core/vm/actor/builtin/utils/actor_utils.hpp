/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::utils {
  using runtime::Runtime;

  class ActorUtils {
   public:
    explicit ActorUtils(Runtime &r) : runtime_(r) {}
    virtual ~ActorUtils() = default;

   protected:
    inline Runtime &getRuntime() const {
      return runtime_;
    }

   private:
    Runtime &runtime_;
  };
}  // namespace fc::vm::actor::builtin::utils
