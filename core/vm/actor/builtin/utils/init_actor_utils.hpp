/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "vm/actor/builtin/utils/actor_utils.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::utils {
  using runtime::Runtime;

  class InitUtils : public ActorUtils {
   public:
    explicit InitUtils(Runtime &r) : ActorUtils(r) {}

    virtual outcome::result<void> assertCaller(bool condition) const = 0;
  };

  using InitUtilsPtr = std::shared_ptr<InitUtils>;

}  // namespace fc::vm::actor::builtin::utils
