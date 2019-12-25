/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/exit_code/exit_code.hpp"

#include <sstream>

#include "common/enum.hpp"
#include "common/visitor.hpp"

using fc::common::to_int;
using fc::vm::exit_code::ExitCode;
using fc::vm::exit_code::RuntimeError;
using fc::vm::exit_code::Success;
using TValue = fc::vm::exit_code::ExitCode::TValue;

bool Success::operator==(const Success &other) const {
  return true;
}

ExitCode::ExitCode() : exit_code_(TValue(Success())) {}

ExitCode::ExitCode(SystemError error_code) : exit_code_(TValue(error_code)) {}

ExitCode::ExitCode(UserDefinedError error_code)
    : exit_code_(TValue(error_code)) {}

ExitCode::ExitCode(
    boost::variant<Success, SystemError, UserDefinedError> exit_code)
    : exit_code_(std::move(exit_code)) {}

ExitCode ExitCode::makeOkExitCode() {
  return ExitCode();
}

ExitCode ExitCode::makeSystemErrorExitCode(SystemError error_code) {
  return ExitCode(error_code);
}

ExitCode ExitCode::makeUserDefinedErrorExitCode(UserDefinedError error_code) {
  return ExitCode(error_code);
}

ExitCode ExitCode::ensureErrorCode(const ExitCode &exit_code) {
  if (exit_code.isSuccess())
    return ExitCode(TValue(SystemError::kRuntimeAPIError));
  return exit_code;
}

bool ExitCode::isSuccess() const {
  return visit_in_place(
      exit_code_,
      [](Success) { return true; },
      [](SystemError) { return false; },
      [](UserDefinedError) { return false; });
}

bool ExitCode::isError() const {
  return !isSuccess();
}

bool ExitCode::allowsStateUpdate() const {
  return isSuccess();
}

std::string ExitCode::toString() const {
  return visit_in_place(
      exit_code_,
      [](Success) { return "Success"; },
      [](SystemError e) { return "SystemError " + std::to_string(to_int(e)); },
      [](UserDefinedError e) {
        return "UserDefinedError " + std::to_string(to_int(e));
      });
}

bool ExitCode::operator==(const ExitCode &other) const {
  return exit_code_ == other.exit_code_;
}

RuntimeError::RuntimeError(ExitCode exit_code, std::string error_message)
    : exit_code_(std::move(exit_code)),
      error_message_(std::move(error_message)) {}

RuntimeError::RuntimeError(ExitCode exit_code)
    : exit_code_(std::move(exit_code)) {}

std::string RuntimeError::toString() const {
  std::stringstream res;
  res << "Runtime error: '" << exit_code_.toString();
  if (!error_message_.empty()) res << " (\"" << error_message_ << "\")";
  return res.str();
}
