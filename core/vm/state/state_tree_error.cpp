/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/state/state_tree_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::vm::state, StateTreeError, e) {
  using fc::vm::state::StateTreeError;

  if (e == StateTreeError::kStateNotFound) {
    return "StateTreeError: state not found";
  }
  return "unknown error";
}
