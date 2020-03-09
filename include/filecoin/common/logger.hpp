/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_LOGGER_HPP
#define CPP_FILECOIN_LOGGER_HPP

#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

namespace fc::common {
  using Logger = std::shared_ptr<spdlog::logger>;

  /**
   * Provide logger object
   * @param tag - tagging name for identifying logger
   * @return logger object
   */
  Logger createLogger(const std::string &tag);
}  // namespace fc::common

#endif  // CPP_FILECOIN_LOGGER_HPP
