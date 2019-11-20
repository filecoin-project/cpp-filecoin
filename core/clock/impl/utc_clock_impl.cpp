/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clock/impl/utc_clock_impl.hpp"

namespace fc::clock {
  // TODO(turuslan): add NTP sync if necessary
  Time UTCClockImpl::nowUTC() const {
    return Time(std::chrono::system_clock::now().time_since_epoch());
  }
}  // namespace fc::clock
