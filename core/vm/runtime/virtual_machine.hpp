/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/runtime/runtime_types.hpp"

namespace fc::vm {
  using message::UnsignedMessage;
  using primitives::TokenAmount;
  using runtime::MessageReceipt;

  struct ApplyRet {
    MessageReceipt receipt;
    TokenAmount penalty;
    TokenAmount reward;
  };

  struct VirtualMachine {
    virtual ~VirtualMachine() = default;
    virtual outcome::result<ApplyRet> applyMessage(
        const UnsignedMessage &message, size_t size) = 0;
    virtual outcome::result<MessageReceipt> applyImplicitMessage(
        const UnsignedMessage &message) = 0;
    virtual outcome::result<CID> flush() = 0;
  };
}  // namespace fc::vm
