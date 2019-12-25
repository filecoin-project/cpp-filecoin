/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/exit_code/exit_code.hpp"

#include <sstream>

#include "common/enum.hpp"

using fc::common::to_int;
using fc::vm::exit_code::ErrorCode;
using fc::vm::exit_code::ExitCode;
using fc::vm::exit_code::RuntimeError;

ExitCode::ExitCode(ErrorCode error_code) : exit_code_{error_code} {}

ExitCode ExitCode::makeOkExitCode() {
  return ExitCode(ErrorCode::kSuccess);
}

ExitCode ExitCode::makeErrorExitCode(ErrorCode error_code) {
  return ExitCode(error_code);
}

ExitCode ExitCode::ensureErrorCode(const ExitCode &exit_code) {
  if (exit_code.isSuccess()) return ExitCode(ErrorCode::kRuntimeAPIError);
  return exit_code;
}

bool ExitCode::isSuccess() const {
  return exit_code_ == ErrorCode::kSuccess;
}

bool ExitCode::isError() const {
  return !isSuccess();
}

bool ExitCode::allowsStateUpdate() const {
  return isSuccess();
}

std::string ExitCode::toString() const {
  if (exit_code_ == ErrorCode::kSuccess) return "Success";
  return "ErrorCode " + std::to_string(to_int(exit_code_));
}

bool ExitCode::operator==(const ExitCode &other) const {
  return exit_code_ == other.exit_code_;
}

RuntimeError::RuntimeError(ExitCode exit_code, std::string error_message)
    : exit_code_(exit_code), error_message_(std::move(error_message)) {}

RuntimeError::RuntimeError(ExitCode exit_code) : exit_code_(exit_code) {}

std::string RuntimeError::toString() const {
  std::stringstream res;
  res << "Runtime error: '" << exit_code_.toString();
  if (!error_message_.empty()) res << " (\"" << error_message_ << "\")";
  return res.str();
}
