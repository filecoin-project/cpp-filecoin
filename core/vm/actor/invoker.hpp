/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/sector/sector.hpp"
#include "primitives/types.hpp"
#include "vm/actor/actor_method.hpp"

namespace fc::vm::actor {
  using primitives::StoragePower;
  using runtime::InvocationOutput;

  /// Finds and loads actor code, invokes actor methods
  class Invoker {
   public:
    virtual ~Invoker() = default;

    virtual outcome::result<InvocationOutput> invoke(
        const Actor &actor, const std::shared_ptr<Runtime> &runtime) = 0;
  };
}  // namespace fc::vm::actor
