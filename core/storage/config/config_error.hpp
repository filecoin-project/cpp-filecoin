/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_STORAGE_CONFIG_ERROR_HPP
#define FILECOIN_CORE_STORAGE_CONFIG_ERROR_HPP

#include "common/outcome.hpp"

namespace fc::storage::config {

  /**
   * @brief FileStore returns these types of errors
   */
  enum class ConfigError {
    kJSONParserError = 1,
    kBadPath,
    kCannotOpenFile,
  };

}  // namespace fc::storage::config

OUTCOME_HPP_DECLARE_ERROR(fc::storage::config, ConfigError);

#endif  // FILECOIN_CORE_STORAGE_CONFIG_ERROR_HPP
