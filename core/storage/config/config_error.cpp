/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/storage/config/config_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::config, ConfigError, e) {
  using fc::storage::config::ConfigError;

  switch (e) {
    case (ConfigError::JSON_PARSER_ERROR):
      return "ConfigError: JSON parser error";
    case (ConfigError::BAD_PATH):
      return "ConfigError: config key is wrong";
    case (ConfigError::CANNOT_OPEN_FILE):
      return "ConfigError: cannot open file";
    default:
      return "ConfigError: unknown error";
  }
}
