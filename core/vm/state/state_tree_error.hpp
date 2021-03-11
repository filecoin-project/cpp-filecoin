/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"

namespace fc::vm::state {

  /**
   * @brief Type of errors returned by StateTree
   */
  enum class StateTreeError {
    kStateNotFound = 1,
  };

}  // namespace fc::vm::state

OUTCOME_HPP_DECLARE_ERROR(fc::vm::state, StateTreeError);
