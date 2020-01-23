/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_INVOKER_IMPL_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_INVOKER_IMPL_HPP

#include "vm/actor/invoker.hpp"

#include "vm/actor/actor_method.hpp"

namespace fc::vm::actor {

  using runtime::InvocationOutput;

  /// Finds and loads actor code, invokes actor methods
  class InvokerImpl : public Invoker {
   public:
    static constexpr VMExitCode CANT_INVOKE_ACCOUNT_ACTOR{254};
    static constexpr VMExitCode NO_CODE_OR_METHOD{255};

    InvokerImpl();
    ~InvokerImpl() override = default;
    outcome::result<InvocationOutput> invoke(
        const Actor &actor,
        const std::shared_ptr<Runtime> &runtime,
        MethodNumber method,
        const MethodParams &params) override;

   private:
    std::map<CID, ActorExports> builtin_;
  };
}  // namespace fc::vm::actor

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_INVOKER_IMPL_HPP
