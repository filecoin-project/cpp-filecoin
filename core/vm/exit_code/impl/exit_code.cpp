/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/exit_code/exit_code.hpp"

#include <spdlog/fmt/fmt.h>
#include <sstream>

OUTCOME_CPP_DEFINE_CATEGORY(fc::vm, VMExitCode, e) {
  if (e == fc::vm::VMExitCode::kFatal) {
    return "VMExitCode::kFatal";
  }
  return fmt::format("VMExitCode vm exit code {}", e);
}

OUTCOME_CPP_DEFINE_CATEGORY(fc::vm, VMFatal, e) {
  return "VMFatal::kFatal fatal vm error";
}

OUTCOME_CPP_DEFINE_CATEGORY(fc::vm, VMAbortExitCode, e) {
  return fmt::format("VMAbortExitCode vm exit code {}", e);
}

namespace fc::vm {
  bool isVMExitCode(const std::error_code &error) {
    return error.category() == __libp2p::Category<VMExitCode>::get();
  }

  bool isFatal(const std::error_code &error) {
    return error.category() == __libp2p::Category<VMFatal>::get();
  }

  bool isAbortExitCode(const std::error_code &error) {
    return error.category() == __libp2p::Category<VMAbortExitCode>::get();
  }

  outcome::result<VMExitCode> asExitCode(const std::error_code &error) {
    if (isVMExitCode(error)) {
      return outcome::success(VMExitCode{error.value()});
    }
    return outcome::failure(error);
  }
}  // namespace fc::vm
