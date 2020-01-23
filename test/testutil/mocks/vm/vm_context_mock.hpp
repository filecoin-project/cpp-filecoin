/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_VM_CONTEXT_MOCK_HPP
#define CPP_FILECOIN_VM_CONTEXT_MOCK_HPP

#include <gmock/gmock.h>
#include "vm/vm_context.hpp"

namespace fc::vm {
  class MockVMContext : public VMContext {
   public:
    MOCK_METHOD0(message, Message());

    MOCK_METHOD4(send,
                 outcome::result<void>(primitives::address::Address,
                                       uint64_t,
                                       actor::BigInt,
                                       std::vector<uint8_t>));
  };
}  // namespace fc::vm

#endif  // CPP_FILECOIN_VM_CONTEXT_MOCK_HPP
