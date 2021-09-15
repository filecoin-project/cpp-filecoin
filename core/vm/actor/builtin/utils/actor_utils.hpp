/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/runtime/runtime.hpp"

#define UTILS_VM_ASSERT(condition) \
  OUTCOME_TRY(getRuntime().vm_assert(condition))

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
