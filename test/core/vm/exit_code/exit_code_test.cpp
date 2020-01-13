/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/exit_code/exit_code.hpp"

#include <gtest/gtest.h>

using fc::vm::exit_code::ErrorCode;
using fc::vm::exit_code::ExitCode;
using fc::vm::exit_code::RuntimeError;

enum SampleError {};
OUTCOME_HPP_DECLARE_ERROR(SampleError);
OUTCOME_CPP_DEFINE_CATEGORY(SampleError, e) {
  return "sample error";
}

/// Distinguish VMExitCode errors from other errors
TEST(VMExitCode, IsVMExitCode) {
  using fc::vm::isVMExitCode;
  using fc::outcome::failure;
  EXPECT_TRUE(isVMExitCode(failure(fc::vm::VMExitCode(1)).error()));
  EXPECT_FALSE(isVMExitCode(failure(SampleError(1)).error()));
}

/**
 * @given success exit codes
 * @when isSuccess() called
 * @then true returned
 */
TEST(ExitCodeTest, SuccessExitCode) {
  ExitCode success1(ErrorCode::kSuccess);
  ExitCode success2 = ExitCode::makeOkExitCode();

  ASSERT_TRUE(success1 == success2);

  ASSERT_TRUE(success1.isSuccess());
  ASSERT_FALSE(success1.isError());
  ASSERT_TRUE(success1.allowsStateUpdate());
}

/**
 * @given System error exit codes
 * @when isError() called
 * @then true returned
 */
TEST(ExitCodeTest, SystemErrorExitCode) {
  ExitCode system_error1(ErrorCode::kActorCodeNotFound);
  ExitCode system_error2 =
      ExitCode::makeErrorExitCode(ErrorCode::kActorCodeNotFound);

  ASSERT_TRUE(system_error1 == system_error2);

  ASSERT_FALSE(system_error1.isSuccess());
  ASSERT_TRUE(system_error1.isError());
  ASSERT_FALSE(system_error1.allowsStateUpdate());
}

/**
 * @given exit_code with error
 * @when ensureErrorCode called
 * @then exit_code returned itself
 */
TEST(ExitCodeTest, ErrorCodeEnsureErrorCode) {
  ExitCode system_error = ExitCode(ErrorCode::kActorNotFound);
  ExitCode runtime_api_error = ExitCode(ErrorCode::kRuntimeAPIError);

  ASSERT_EQ(system_error, ExitCode::ensureErrorCode(system_error));
  ASSERT_EQ(runtime_api_error, ExitCode::ensureErrorCode(runtime_api_error));
}

/**
 * @given exit_code with success
 * @when ensureErrorCode called
 * @then ExitCode with system error kRuntimeAPIError returned
 */
TEST(ExitCodeTest, SuccessEnsureErrorCode) {
  ExitCode success(ErrorCode::kSuccess);
  ExitCode runtime_api_error = ExitCode(ErrorCode::kRuntimeAPIError);

  ASSERT_EQ(runtime_api_error, ExitCode::ensureErrorCode(success));
}

class ExitCodeToStringTest
    : public ::testing::TestWithParam<std::pair<std::string, ExitCode>> {};

/**
 * @given RuntimeError
 * @when to string called
 * @then valid error message returned
 */
TEST_P(ExitCodeToStringTest, ToString) {
  const auto &[expected, exit_code] = GetParam();

  ASSERT_EQ(expected, exit_code.toString());

  RuntimeError empty_error(exit_code);
  std::stringstream expected_empty_msg;
  expected_empty_msg << "Runtime error: '" << exit_code.toString();
  ASSERT_EQ(expected_empty_msg.str(), empty_error.toString());

  std::string error_message = "error message";
  RuntimeError error(exit_code, error_message);
  std::stringstream expected_msg;
  expected_msg << "Runtime error: '" << exit_code.toString() << " (\""
               << error_message << "\")";
  ASSERT_EQ(expected_msg.str(), error.toString());
}

INSTANTIATE_TEST_CASE_P(
    ExitCodeToStringTest,
    ExitCodeToStringTest,
    ::testing::Values(
        std::make_pair("Success", ExitCode(ErrorCode::kSuccess)),
        std::make_pair("Success", ExitCode::makeOkExitCode()),
        std::make_pair("ErrorCode 2", ExitCode(ErrorCode::kActorCodeNotFound)),
        std::make_pair(
            "ErrorCode 2",
            ExitCode::makeErrorExitCode(ErrorCode::kActorCodeNotFound)),
        std::make_pair("ErrorCode 5",
                       ExitCode(ErrorCode::kInsufficientFundsSystem))));
