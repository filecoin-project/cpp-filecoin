/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_INVOKER_IMPL_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_INVOKER_IMPL_HPP

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
    std::map<CID, ActorExports> builtin_;
  };
}  // namespace fc::vm::actor

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_INVOKER_IMPL_HPP
