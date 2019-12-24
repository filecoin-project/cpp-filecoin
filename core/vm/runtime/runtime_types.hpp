/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_RUNTIME_RUNTIME_TYPES_HPP
#define CPP_FILECOIN_CORE_VM_RUNTIME_RUNTIME_TYPES_HPP

#include "common/buffer.hpp"
#include "primitives/address.hpp"
#include "primitives/big_int.hpp"
#include "vm/actor/actor.hpp"
#include "vm/exit_code/exit_code.hpp"

namespace fc::vm::runtime {

  using actor::ActorSubstateCID;
  using actor::MethodNumber;
  using actor::MethodParams;
  using actor::TokenAmount;
  using exit_code::ExitCode;
  using primitives::Address;
  using primitives::BigInt;

  struct InvocationInput {
    Address to;
    MethodNumber method;
    MethodParams params;
    TokenAmount Value;
  };

  struct InvocationOutput {
    common::Buffer return_value;
  };

  enum class ComputeFunctionID {
    VERIFY_SIGNATURE
  }

  // Interface
  class ActorStateHandle {
    virtual void updateRelease(ActorSubstateCID new_state_cid) = 0;
    virtual void release(ActorSubstateCID check_state_cid) = 0;
    virtual ActorSubstateCID take() = 0;
  }

  struct MessageReceipt {
    ExitCode exit_code;
    Buffer return_value;
    // TODO(a.chernyshov) gas amount type
    BigInt gas_used;
  };

}  // namespace fc::vm::runtime

#endif  // CPP_FILECOIN_CORE_VM_RUNTIME_RUNTIME_TYPES_HPP
