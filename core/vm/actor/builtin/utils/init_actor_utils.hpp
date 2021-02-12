/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::utils::init {
    using runtime::Runtime;

  class InitUtils {
      public:
      InitUtils(Runtime &r) : runtime(r) {}
      virtual ~InitUtils() = default;

      virtual outcome::result<void> assertCaller(bool condition) const = 0;

      protected:
      Runtime &runtime;
  };

  using InitUtilsPtr = std::shared_ptr<InitUtils>;

} // namespace fc::vm::actor::builtin::utils::init
