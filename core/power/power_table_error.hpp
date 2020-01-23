/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_POWER_POWER_TABLE_ERROR_HPP
#define FILECOIN_CORE_POWER_POWER_TABLE_ERROR_HPP

#include "common/outcome.hpp"

namespace fc::power {

  /**
   * @brief Type of errors returned by Power Table
   */
  enum class PowerTableError {
    NO_SUCH_MINER = 1,
    NEGATIVE_POWER
  };

}  // namespace fc::storage::power

OUTCOME_HPP_DECLARE_ERROR(fc::power, PowerTableError);

#endif  // FILECOIN_CORE_POWER_POWER_TABLE_ERROR_HPP
