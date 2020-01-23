/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "power/power_table_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::power, PowerTableError, e) {
  using fc::power::PowerTableError;

  switch (e) {
    case PowerTableError::NO_SUCH_MINER:
      return "PowerTableError: miner not found";
    case PowerTableError::NEGATIVE_POWER:
      return "PowerTableError: power cannot be negative";
  }

  return "unknown error";
}
