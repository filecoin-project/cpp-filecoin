/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/todo_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc, TodoError, e) {
  using E = fc::TodoError;
  switch (e) {
    case E::kError:
      return "Error";
  }
}
