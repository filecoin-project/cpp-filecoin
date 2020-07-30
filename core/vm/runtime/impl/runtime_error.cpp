/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/runtime_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::vm::runtime, RuntimeError, e) {
  using fc::vm::runtime::RuntimeError;

  switch (e) {
    case RuntimeError::kWrongAddressProtocol:
      return "RuntimeError: wrong address protocol";
    case RuntimeError::kCreateActorOperationNotPermitted:
      return "RuntimeError: create actor operation is not permitted";
    case RuntimeError::kDeleteActorOperationNotPermitted:
      return "RuntimeError: delete actor operation is not permitted";
    case RuntimeError::kActorNotFound:
      return "RuntimeError: receiver actor not found";
    case RuntimeError::kNotEnoughFunds:
      return "RuntimeError: not enough funds";
    case RuntimeError::kNotEnoughGas:
      return "RuntimeError: not enough gas";
    default:
      return "unknown error";
  }
}
