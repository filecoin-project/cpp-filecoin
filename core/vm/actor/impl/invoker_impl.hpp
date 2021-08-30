/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/invoker.hpp"

#include "vm/actor/actor_method.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor {
  using runtime::InvocationOutput;
  using runtime::Runtime;

  /// Finds and loads actor code, invokes actor methods
  class InvokerImpl : public Invoker {
   public:
    InvokerImpl();

    ~InvokerImpl() override = default;

    outcome::result<InvocationOutput> invoke(
        const Actor &actor, const std::shared_ptr<Runtime> &runtime) override;

   private:
    std::map<ActorCodeCid, ActorExports> builtin_;

    // Temp for miner actor
    std::set<MethodNumber> ready_miner_actor_methods_v0;
    std::set<MethodNumber> ready_miner_actor_methods_v2;
    std::set<MethodNumber> ready_miner_actor_methods_v3;
  };
}  // namespace fc::vm::actor
