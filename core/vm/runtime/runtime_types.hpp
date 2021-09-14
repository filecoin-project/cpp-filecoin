/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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
  using primitives::ChainEpoch;
  using primitives::GasAmount;
  using primitives::address::Address;

  using InvocationOutput = Buffer;

  /**
   * Id of native function
   */
  enum class ComputeFunctionID { VERIFY_SIGNATURE };

  /**
   * Result of message execution
   */
  struct MessageReceipt {
    VMExitCode exit_code{};
    Buffer return_value;
    GasAmount gas_used{};
  };

  CBOR_TUPLE(MessageReceipt, exit_code, return_value, gas_used)

  struct ExecutionResult {
    UnsignedMessage message;
    MessageReceipt receipt;
    std::string error;
  };

  enum class ConsensusFaultType {
    DoubleForkMining = 1,
    ParentGrinding = 2,
    TimeOffsetMining = 3,
  };

  struct ConsensusFault {
    Address target;
    ChainEpoch epoch{};
    ConsensusFaultType type{};
  };
}  // namespace fc::vm::runtime
