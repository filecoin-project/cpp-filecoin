/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "vm/interpreter/impl/interpreter_impl.hpp"

namespace fc::vm::interpreter {
  class InterpreterMock : public Interpreter {
   public:
    MOCK_CONST_METHOD2(interpret,
                       outcome::result<Result>(TsBranchPtr,
                                               const TipsetCPtr &tipset));
  };
}  // namespace fc::vm::interpreter
