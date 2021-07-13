/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clock/impl/utc_clock_impl.hpp"

namespace fc::clock {
  microseconds UTCClockImpl::nowMicro() const {
    return std::chrono::duration_cast<microseconds>(
        std::chrono::system_clock::now().time_since_epoch());
  }
}  // namespace fc::clock
