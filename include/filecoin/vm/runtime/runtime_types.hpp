/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_RUNTIME_RUNTIME_TYPES_HPP
#define CPP_FILECOIN_CORE_VM_RUNTIME_RUNTIME_TYPES_HPP

#include "filecoin/codec/cbor/streams_annotation.hpp"
#include "filecoin/common/buffer.hpp"
#include "filecoin/primitives/address/address.hpp"
#include "filecoin/primitives/big_int.hpp"
#include "filecoin/vm/exit_code/exit_code.hpp"

namespace fc::vm::runtime {

  using common::Buffer;
  using exit_code::ExitCode;
  using primitives::BigInt;

  /**
   * Value returned by method invocation
   */
  struct InvocationOutput {
    common::Buffer return_value;
  };

  inline bool operator==(const InvocationOutput &lhs,
                         const InvocationOutput &rhs) {
    return lhs.return_value == rhs.return_value;
  }

  /**
   * Id of native function
   */
  enum class ComputeFunctionID { VERIFY_SIGNATURE };

  /**
   * Result of message execution
   */
  struct MessageReceipt {
    uint8_t exit_code;
    Buffer return_value;
    BigInt gas_used;
  };

  CBOR_TUPLE(MessageReceipt, exit_code, return_value, gas_used)
}  // namespace fc::vm::runtime

#endif  // CPP_FILECOIN_CORE_VM_RUNTIME_RUNTIME_TYPES_HPP
