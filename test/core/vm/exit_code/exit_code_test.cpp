/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/exit_code/exit_code.hpp"

#include <gtest/gtest.h>

enum SampleError {
  kSample = 1,
};
OUTCOME_HPP_DECLARE_ERROR(SampleError);
OUTCOME_CPP_DEFINE_CATEGORY(SampleError, e) {
  return "sample error";
}

namespace fc::vm {
  using fc::outcome::failure;

  /// Distinguish VMExitCode errors from other errors
  TEST(VMExitCode, IsVMExitCode) {
    EXPECT_TRUE(
        isVMExitCode(failure(VMExitCode(SampleError::kSample)).error()));
    EXPECT_FALSE(isVMExitCode(failure(SampleError::kSample).error()));
  }

  /// Distinguish VMFatal errors from other errors
  TEST(VMExitCode, IsFatal) {
    EXPECT_TRUE(isFatal(failure(VMFatal(VMFatal::kFatal)).error()));
    EXPECT_FALSE(isFatal(failure(SampleError::kSample).error()));
  }

  /**
   * @given VMExitCode exit_code
   * @when requireNoError called
   * @then exit_code converted to VMAbortExitCode and code is preserved
   */
  TEST(VMExitCode, RequireNoErrorVMExitCodeToAbort) {
    VMExitCode exit_code{VMExitCode::kSysErrReserved1};

    outcome::result<void> req{exit_code};
    auto res = requireNoError(req, VMExitCode::kOk);
    EXPECT_TRUE(isAbortExitCode(res.error()));
    EXPECT_FALSE(isVMExitCode(res.error()));

    // convert to VMExitCode
    auto vm_exit_code = VMExitCode{res.error().value()};
    EXPECT_TRUE(isVMExitCode(vm_exit_code));
    EXPECT_EQ(exit_code, vm_exit_code);
  }

  /**
   * @given VMFatal error
   * @when requireNoError called
   * @then fatal error returned
   */
  TEST(VMExitCode, RequireNoErrorFatal) {
    outcome::result<void> req{VMFatal::kFatal};
    auto res = requireNoError(req, VMExitCode::kOk);
    EXPECT_TRUE(isFatal(res.error()));
  }

  /**
   * @given non VMExitCode error
   * @when requireNoError called
   * @then default VMExitCode error returned
   */
  TEST(VMExitCode, RequireNoErrorDefault) {
    VMExitCode default_exit_code{VMExitCode::kOk};

    outcome::result<void> req{SampleError::kSample};
    auto res = requireNoError(req, default_exit_code);
    EXPECT_TRUE(isAbortExitCode(res.error()));
    EXPECT_FALSE(isVMExitCode(res.error()));

    // convert to VMExitCode
    auto vm_exit_code = VMExitCode{res.error().value()};
    EXPECT_TRUE(isVMExitCode(vm_exit_code));
    EXPECT_EQ(vm_exit_code, default_exit_code);
  }

}  // namespace fc::vm
