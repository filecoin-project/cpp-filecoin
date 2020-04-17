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

/// Distinguish VMExitCode errors from other errors
TEST(VMExitCode, IsVMExitCode) {
  using fc::vm::isVMExitCode;
  using fc::outcome::failure;
  EXPECT_TRUE(isVMExitCode(failure(fc::vm::VMExitCode(1)).error()));
  EXPECT_FALSE(isVMExitCode(failure(SampleError(1)).error()));
}
