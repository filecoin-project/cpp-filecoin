/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/config/config_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::config, ConfigError, e) {
  using fc::storage::config::ConfigError;

  switch (e) {
    case (ConfigError::kJSONParserError):
      return "ConfigError: JSON parser error";
    case (ConfigError::kBadPath):
      return "ConfigError: config key is wrong";
    case (ConfigError::kCannotOpenFile):
      return "ConfigError: cannot open file";
    default:
      return "ConfigError: unknown error";
  }
}
