/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clock/impl/utc_clock_impl.hpp"

namespace fc::clock {
  // TODO(turuslan): add NTP sync if necessary
  UnixTime UTCClockImpl::nowUTC() const {
    return std::chrono::duration_cast<UnixTime>(
        std::chrono::system_clock::now().time_since_epoch());
  }

  Microseconds UTCClockImpl::microsecSinceEpoch() const {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
  }
}  // namespace fc::clock
