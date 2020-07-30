/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_COMMON_TODO_ERROR_HPP
#define CPP_FILECOIN_CORE_COMMON_TODO_ERROR_HPP

#include "common/outcome.hpp"

namespace fc {
  enum class TodoError {
    kError = 1,
  };
}  // namespace fc

OUTCOME_HPP_DECLARE_ERROR(fc, TodoError);

#endif  // CPP_FILECOIN_CORE_COMMON_TODO_ERROR_HPP
