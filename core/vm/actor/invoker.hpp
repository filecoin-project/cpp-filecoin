/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_INVOKER_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_INVOKER_HPP

#include "vm/actor/actor_method.hpp"

namespace fc::vm::actor {
  class Invoker {
   public:
    Invoker();
    outcome::result<Buffer> invoke(const Actor &actor,
                                   VMContext &context,
                                   uint64_t method,
                                   gsl::span<const uint8_t> params);

   private:
    std::map<CID, ActorExports> builtin_;
  };
}  // namespace fc::vm::actor

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_INVOKER_HPP
