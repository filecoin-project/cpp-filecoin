/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CLOCK_IMPL_UTC_CLOCK_IMPL_HPP
#define CPP_FILECOIN_CORE_CLOCK_IMPL_UTC_CLOCK_IMPL_HPP

#include "filecoin/clock/utc_clock.hpp"

namespace fc::clock {
  class UTCClockImpl : public UTCClock {
   public:
    Time nowUTC() const override;
  };
}  // namespace fc::clock

#endif  // CPP_FILECOIN_CORE_CLOCK_IMPL_UTC_CLOCK_IMPL_HPP
