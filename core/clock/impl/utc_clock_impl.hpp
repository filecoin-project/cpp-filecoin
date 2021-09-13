/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "clock/utc_clock.hpp"

namespace fc::clock {
  class UTCClockImpl : public UTCClock {
   public:
    microseconds nowMicro() const override;
  };
}  // namespace fc::clock
