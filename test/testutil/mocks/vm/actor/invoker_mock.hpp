/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>
#include "vm/actor/invoker.hpp"

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
