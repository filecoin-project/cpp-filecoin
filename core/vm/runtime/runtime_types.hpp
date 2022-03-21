/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/streams_annotation.hpp"
#include "common/bytes.hpp"
#include "primitives/address/address.hpp"
#include "primitives/big_int.hpp"
#include "vm/exit_code/exit_code.hpp"
#include "vm/message/message.hpp"
#include "vm/runtime/consensus_fault_types.hpp"

namespace fc::vm::runtime {
  using message::UnsignedMessage;
  using primitives::BigInt;
  using primitives::ChainEpoch;
  using primitives::GasAmount;
  using primitives::address::Address;

  using InvocationOutput = Bytes;

  /**
   * Id of native function
   */
  enum class ComputeFunctionID { VERIFY_SIGNATURE };

  /**
   * Result of message execution
   */
  struct MessageReceipt {
    VMExitCode exit_code{};
    Bytes return_value;
    GasAmount gas_used{};
  };

  CBOR_TUPLE(MessageReceipt, exit_code, return_value, gas_used)

  struct ExecutionResult {
    UnsignedMessage message;
    MessageReceipt receipt;
    std::string error;
  };
}  // namespace fc::vm::runtime
