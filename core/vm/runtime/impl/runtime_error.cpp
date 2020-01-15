/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/runtime_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::vm::runtime, RuntimeError, e) {
  using fc::vm::runtime::RuntimeError;

  switch (e) {
    case RuntimeError::WRONG_ADDRESS_PROTOCOL:
      return "RuntimeError: wrong address protocol";
    case RuntimeError::CREATE_ACTOR_OPERATION_NOT_PERMITTED:
      return "RuntimeError: create actor operation is not permitted";
    case RuntimeError::DELETE_ACTOR_OPERATION_NOT_PERMITTED:
      return "RuntimeError: delete actor operation is not permitted";
    case RuntimeError::ACTOR_NOT_FOUND:
      return "RuntimeError: receiver actor not found";
    case RuntimeError::UNKNOWN:
      break;
  }

  return "unknown error";
}
