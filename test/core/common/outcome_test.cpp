/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/outcome.hpp"

#include <gtest/gtest.h>

#include "common/error_text.hpp"

namespace fc {

  const int ret = 42;

  constexpr auto error_message{"meaningful text"};
  const auto error{ERROR_TEXT(error_message)};

  outcome::result<void> funcSuccess() {
    return outcome::success();
  }

  outcome::result<void> funcFailure() {
    return error;
  }

  outcome::result<int> funcSuccessReturn() {
    return ret;
  }

  outcome::result<int> funcFailureReturn() {
    return error;
  }

  /**
   * No throw on success result with no value
   */
  TEST(OutcomeExcept, OneArgNoExcept) {
    EXPECT_NO_THROW(OUTCOME_EXCEPT(funcSuccess()));
  }

  /**
   * Throw on failure result with no value
   */
  TEST(OutcomeExcept, OneArgExcept) {
    EXPECT_THROW(OUTCOME_EXCEPT(funcFailure()), std::system_error);
  }

  /**
   * No throw on success result with value returned
   */
  TEST(OutcomeExcept, ValueReturnedNoExcept) {
    OUTCOME_EXCEPT(res, funcSuccessReturn());
    EXPECT_EQ(res, ret);
  }

  /**
   * Throw on failure result with value returned
   */
  TEST(OutcomeExcept, ValueReturnedExcept) {
    EXPECT_THROW(OUTCOME_EXCEPT(funcFailureReturn()), std::system_error);
  }

  TEST(ErrorText, ErrorText) {
    EXPECT_EQ(error.message(), error_message);
  }

  /**
   * error_code value is not zero
   */
  TEST(ErrorText, ErrorTextZero) {
    EXPECT_DEATH(error_text::_make_error_code((const char *)nullptr), "");
    EXPECT_TRUE(error_text::_make_error_code(
        (const char *)(uintptr_t{1}
                       << std::numeric_limits<unsigned int>::digits)));
  }
}  // namespace fc
