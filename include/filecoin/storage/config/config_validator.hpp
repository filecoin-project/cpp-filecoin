/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_CONFIG_VALIDATOR_HPP
#define CPP_FILECOIN_CORE_STORAGE_CONFIG_VALIDATOR_HPP

#include "filecoin/common/outcome.hpp"
#include "filecoin/storage/config/config.hpp"

namespace fc::storage::config {

  /**
   * @brief Interface for config validator. Inherit for particular configuration
   * validation.
   */
  class ConfigValidator {
   public:
    virtual ~ConfigValidator() = default;

    virtual fc::outcome::result<bool> Validate(const Config &config) = 0;
  };

}  // namespace fc::storage::config

#endif  // CPP_FILECOIN_CORE_STORAGE_CONFIG_VALIDATOR_HPP
