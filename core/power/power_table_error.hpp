/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"

namespace fc::power {

  /**
   * @brief Type of errors returned by Power Table
   */
  enum class PowerTableError { kNoSuchMiner = 1, kNegativePower };

}  // namespace fc::power

OUTCOME_HPP_DECLARE_ERROR(fc::power, PowerTableError);
