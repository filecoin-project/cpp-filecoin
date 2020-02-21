/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/exit_code/exit_code.hpp"

#include <sstream>

#include <boost/assert.hpp>

#include "common/enum.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::vm, VMExitCode, e) {
  return "vm exit code";
}

namespace fc::vm {
  bool isVMExitCode(const std::error_code &error) {
    return error.category() == __libp2p::Category<VMExitCode>::get();
  }

  uint8_t getRetCode(VMExitCode error) {
    using E = VMExitCode;
    switch (error) {
      case E::_:
        break;

      case E::DECODE_ACTOR_PARAMS_ERROR:
      case E::ENCODE_ACTOR_PARAMS_ERROR:
        return 1;

      case E::INVOKER_CANT_INVOKE_ACCOUNT_ACTOR:
        return 254;
      case E::INVOKER_NO_CODE_OR_METHOD:
        return 255;

      case E::ACCOUNT_ACTOR_CREATE_WRONG_ADDRESS_TYPE:
      case E::ACCOUNT_ACTOR_RESOLVE_NOT_FOUND:
      case E::ACCOUNT_ACTOR_RESOLVE_NOT_ACCOUNT_ACTOR:
        return 1;

      case E::MULTISIG_ACTOR_WRONG_CALLER:
        return 1;
      case E::MULTISIG_ACTOR_ILLEGAL_ARGUMENT:
        return 16;  // ErrIllegalArgument in actor-specs
      case E::MULTISIG_ACTOR_NOT_FOUND:
        return 17;  // ErrNotFound in actor-specs
      case E::MULTISIG_ACTOR_FORBIDDEN:
        return 18;  // ErrForbidden in actor-specs
      case E::MULTISIG_ACTOR_INSUFFICIENT_FUNDS:
        return 19;  // ErrInsufficientFunds in actor-specs
      case E::MULTISIG_ACTOR_ILLEGAL_STATE:
        return 20;  // ErrIllegalState in actor-specs

      case E::PAYMENT_CHANNEL_WRONG_CALLER:
        return 1;
      case E::PAYMENT_CHANNEL_ILLEGAL_ARGUMENT:
        return 16;  // ErrIllegalArgument in actor-specs

      // TODO(turuslan): FIL-128 StoragePowerActor
      case E::STORAGE_POWER_ACTOR_WRONG_CALLER:
      case E::STORAGE_POWER_ACTOR_OUT_OF_BOUND:
      case E::STORAGE_POWER_ACTOR_ALREADY_EXISTS:
      case E::STORAGE_POWER_DELETION_ERROR:
        break;
      case E::STORAGE_POWER_ILLEGAL_ARGUMENT:
        return 16;  // ErrIllegalArgument in actor-specs
      case E::STORAGE_POWER_FORBIDDEN:
        return 18;  // ErrForbidden in actor-specs

      case E::INIT_ACTOR_NOT_BUILTIN_ACTOR:
      case E::INIT_ACTOR_SINGLETON_ACTOR:
        return 1;

      case E::CRON_ACTOR_WRONG_CALL:
        return 1;

      case E::REWARD_ACTOR_NEGATIVE_WITHDRAWABLE:
      case E::REWARD_ACTOR_WRONG_CALLER:
        return 1;
    }
    BOOST_ASSERT_MSG(false, "Ret code mapping missing");
  }

  outcome::result<uint8_t> getRetCode(const std::error_code &error) {
    if (!isVMExitCode(error)) {
      return error;
    }
    return getRetCode(VMExitCode(error.value()));
  }
}  // namespace fc::vm

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
