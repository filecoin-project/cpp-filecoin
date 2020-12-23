/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/exit_code/exit_code.hpp"

#include <gtest/gtest.h>

enum SampleError {};
OUTCOME_HPP_DECLARE_ERROR(SampleError);
OUTCOME_CPP_DEFINE_CATEGORY(SampleError, e) {
  return "sample error";
}

namespace fc::vm {
  using fc::outcome::failure;

  /// Distinguish VMExitCode errors from other errors
  TEST(VMExitCode, IsVMExitCode) {
    EXPECT_TRUE(isVMExitCode(failure(VMExitCode(1)).error()));
    EXPECT_FALSE(isVMExitCode(failure(SampleError(1)).error()));
  }

  /// Distinguish VMFatal errors from other errors
  TEST(VMExitCode, IsFatal) {
    EXPECT_TRUE(isFatal(failure(VMFatal(VMFatal::kFatal)).error()));
    EXPECT_FALSE(isFatal(failure(SampleError(1)).error()));
  }

}  // namespace fc::vm
