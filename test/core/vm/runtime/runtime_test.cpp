/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"
#include "vm/exit_code/exit_code.hpp"

enum SampleError {};
OUTCOME_HPP_DECLARE_ERROR(SampleError);
OUTCOME_CPP_DEFINE_CATEGORY(SampleError, e) {
  return "sample error";
}

namespace fc::vm::runtime {
  using fc::outcome::failure;

  /**
   * @given VMExitCode exit_code
   * @when requireNoError called
   * @then exit_code converted to VMAbortExitCode and code is preserved
   */
  TEST(RuntimeTest, RequireNoErrorVMExitCodeToAbort) {
    MockRuntime runtime;
    VMExitCode exit_code{VMExitCode::kSysErrInternal};

    outcome::result<void> req{exit_code};
    auto res = runtime.requireNoError(req, VMExitCode::kOk);
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
  TEST(RuntimeTest, RequireNoErrorFatal) {
    MockRuntime runtime;

    outcome::result<void> req{VMFatal::kFatal};
    auto res = runtime.requireNoError(req, VMExitCode::kOk);
    EXPECT_TRUE(isFatal(res.error()));
  }

  /**
   * @given non VMExitCode error
   * @when requireNoError called
   * @then default VMExitCode error returned
   */
  TEST(RuntimeTest, RequireNoErrorDefault) {
    MockRuntime runtime;
    VMExitCode default_exit_code{VMExitCode::kOk};

    outcome::result<void> req{SampleError(1)};
    auto res = runtime.requireNoError(req, default_exit_code);
    EXPECT_TRUE(isAbortExitCode(res.error()));
    EXPECT_FALSE(isVMExitCode(res.error()));

    // convert to VMExitCode
    auto vm_exit_code = VMExitCode{res.error().value()};
    EXPECT_TRUE(isVMExitCode(vm_exit_code));
    EXPECT_EQ(vm_exit_code, default_exit_code);
  }

}  // namespace fc::vm::runtime
