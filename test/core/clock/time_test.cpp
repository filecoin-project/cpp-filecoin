/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/clock/time.hpp"

#include <gtest/gtest.h>
#include "testutil/outcome.hpp"

using fc::clock::Time;
using fc::clock::UnixTime;
using fc::clock::UnixTimeNano;

static std::string kValidStr = "2019-10-21T23:12:37.859375345Z";

/**
 * @given arbitrary string
 * @when fromString
 * @then error
 */
TEST(Time, FromStringInvalidFormat) {
  auto from_string = Time::fromString("invalid format");

  EXPECT_OUTCOME_FALSE_1(from_string);
}

/**
 * @given iso time string, non-nanosecond precision
 * @when fromString
 * @then error
 */
TEST(Time, FromStringInvalidFormatNoNano) {
  auto from_string = Time::fromString(kValidStr.substr(0, kValidStr.size() - 1 - 3));

  EXPECT_OUTCOME_FALSE_1(from_string);
}

/**
 * @given iso time string, not ending with "Z"
 * @when fromString
 * @then error
 */
TEST(Time, FromStringInvalidFormatNoZ) {
  auto from_string = Time::fromString(kValidStr.substr(0, kValidStr.size() - 1));

  EXPECT_OUTCOME_FALSE_1(from_string);
}

/**
 * @given iso time string
 * @when fromString
 * @then success
 */
TEST(Time, FromStringValid) {
  auto from_string = Time::fromString(kValidStr);

  EXPECT_OUTCOME_TRUE_1(from_string);
}

/**
 * @given Time constructed from time string
 * @when time
 * @then equals to original
 */
TEST(Time, TimeStrSame) {
  auto str = Time::fromString(kValidStr).value().time();

  EXPECT_EQ(str, kValidStr);
}

/**
 * @given Time constructed from unix time
 * @when unixTime
 * @then equals to original
 */
TEST(Time, UnixTime) {
  auto unix_time = UnixTime(1);
  auto time = Time(UnixTimeNano(unix_time));

  EXPECT_EQ(time.unixTime(), unix_time);
}

/**
 * @given Time constructed from unix time nano
 * @when unixTimeNano
 * @then equals to original
 */
TEST(Time, UnixTimeNano) {
  auto unix_time_nano = UnixTimeNano(1);
  auto time = Time(unix_time_nano);

  EXPECT_EQ(time.unixTimeNano(), unix_time_nano);
}
