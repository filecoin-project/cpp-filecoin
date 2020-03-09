/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_INVOKER_MOCK_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_INVOKER_MOCK_HPP

#include <gmock/gmock.h>
#include "filecoin/vm/actor/invoker.hpp"

namespace fc::vm::actor {

  class MockInvoker : public Invoker {
   public:
    MOCK_METHOD4(invoke,
                 outcome::result<InvocationOutput>(const Actor &actor,
                                                   Runtime &runtime,
                                                   MethodNumber method,
                                                   const MethodParams &params));
  };

}  // namespace fc::vm::actor

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_INVOKER_MOCK_HPP
