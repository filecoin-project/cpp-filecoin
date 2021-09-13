/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>
#include <string>

#include "common/outcome.hpp"

namespace fc::clock {
  enum class TimeFromStringError { kInvalidFormat = 1 };

  using UnixTime = std::chrono::seconds;
  using std::chrono::microseconds;
  using Time = UnixTime;

  std::string unixTimeToString(UnixTime);
  outcome::result<UnixTime> unixTimeFromString(const std::string &str);
}  // namespace fc::clock

OUTCOME_HPP_DECLARE_ERROR(fc::clock, TimeFromStringError);
