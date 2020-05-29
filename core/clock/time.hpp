/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CLOCK_TIME_HPP
#define CPP_FILECOIN_CORE_CLOCK_TIME_HPP

#include <chrono>
#include <string>

#include "common/outcome.hpp"

namespace fc::clock {
  enum class TimeFromStringError { INVALID_FORMAT = 1 };

  using UnixTime = std::chrono::seconds;
  using Time = UnixTime;

  std::string unixTimeToString(UnixTime);
  outcome::result<UnixTime> unixTimeFromString(const std::string &str);
}  // namespace fc::clock

OUTCOME_HPP_DECLARE_ERROR(fc::clock, TimeFromStringError);

#endif  // CPP_FILECOIN_CORE_CLOCK_TIME_HPP
