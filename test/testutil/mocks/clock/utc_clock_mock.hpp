/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_CORE_BLOCKCHAIN_MOCKS_UTC_CLOCK
#define TEST_CORE_BLOCKCHAIN_MOCKS_UTC_CLOCK

#include <gmock/gmock.h>

#include "clock/utc_clock.hpp"

namespace fc::clock {
  class UTCClockMock : public UTCClock {
   public:
    MOCK_CONST_METHOD0(nowMicro, microseconds());
  };
}  // namespace fc::clock

#endif
