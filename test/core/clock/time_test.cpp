/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clock/time.hpp"

#include <gtest/gtest.h>

#include "testutil/outcome.hpp"

using fc::clock::UnixTime;
using fc::clock::unixTimeFromString;
using fc::clock::unixTimeToString;

static std::string kValidStr = "2019-10-21T23:12:37Z";

/**
 * @given arbitrary string
 * @when fromString
 * @then error
 */
TEST(Time, FromStringInvalidFormat) {
  EXPECT_OUTCOME_FALSE_1(unixTimeFromString("invalid format"));
}

/**
 * @given iso time string, not ending with "Z"
 * @when fromString
 * @then error
 */
TEST(Time, FromStringInvalidFormatNoZ) {
  EXPECT_OUTCOME_FALSE_1(
      unixTimeFromString(kValidStr.substr(0, kValidStr.size() - 1)));
}

/**
 * @given iso time string
 * @when fromString
 * @then success
 */
TEST(Time, FromStringValid) {
  EXPECT_OUTCOME_TRUE_1(unixTimeFromString(kValidStr));
}

/**
 * @given Time constructed from time string
 * @when time
 * @then equals to original
 */
TEST(Time, TimeStrSame) {
  auto str = unixTimeToString(unixTimeFromString(kValidStr).value());

  EXPECT_EQ(str, kValidStr);
}
