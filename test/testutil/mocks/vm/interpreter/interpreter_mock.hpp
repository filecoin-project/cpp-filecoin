/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_MOCKS_VM_INTERPRETER
#define CPP_FILECOIN_MOCKS_VM_INTERPRETER

#include <gmock/gmock.h>

#include "vm/interpreter/impl/interpreter_impl.hpp"

namespace fc::vm::interpreter {
  class InterpreterMock : public Interpreter {
   public:
    MOCK_CONST_METHOD2(interpret,
                       outcome::result<Result>(const IpldPtr &store,
                                               const Tipset &tipset));
  };
}  // namespace fc::vm::interpreter

#endif
