/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_RUNTIME_RUNTIME_TYPES_HPP
#define CPP_FILECOIN_CORE_VM_RUNTIME_RUNTIME_TYPES_HPP

#include "codec/cbor/streams_annotation.hpp"
#include "common/buffer.hpp"
#include "primitives/address/address.hpp"
#include "primitives/big_int.hpp"
#include "vm/exit_code/exit_code.hpp"
#include "vm/message/message.hpp"

namespace fc::vm::runtime {

  using common::Buffer;
  using message::UnsignedMessage;
  using primitives::BigInt;
  using primitives::GasAmount;

  using InvocationOutput = Buffer;

  /**
   * Id of native function
   */
  enum class ComputeFunctionID { VERIFY_SIGNATURE };

  /**
   * Result of message execution
   */
  struct MessageReceipt {
    VMExitCode exit_code;
    Buffer return_value;
    GasAmount gas_used;
  };

  CBOR_TUPLE(MessageReceipt, exit_code, return_value, gas_used)

  struct ExecutionResult {
    UnsignedMessage message;
    MessageReceipt receipt;
    std::string error;
  };
}  // namespace fc::vm::runtime

#endif  // CPP_FILECOIN_CORE_VM_RUNTIME_RUNTIME_TYPES_HPP
