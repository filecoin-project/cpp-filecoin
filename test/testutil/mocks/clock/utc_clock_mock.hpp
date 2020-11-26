/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_CORE_BLOCKCHAIN_MOCKS_UTC_CLOCK
#define TEST_CORE_BLOCKCHAIN_MOCKS_UTC_CLOCK

#include <gmock/gmock.h>

#include "clock/utc_clock.hpp"

using fc::clock::Time;
using fc::clock::UTCClock;

class UTCClockMock : public UTCClock {
 public:
  MOCK_CONST_METHOD0(nowUTC, Time());
  MOCK_CONST_METHOD0(microsecSinceEpoch, fc::clock::Microseconds());
};

#endif
