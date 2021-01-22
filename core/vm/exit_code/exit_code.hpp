/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/optional.hpp>
#include "common/outcome.hpp"

/**
 * Aborts execution if res has error and aborts with default_error if res has
 * error and the error is not fatal or abort
 */
#define REQUIRE_NO_ERROR(expr, err_code) \
  OUTCOME_TRY(requireNoError((expr), (err_code)));

/**
 * Return VMExitCode as VMAbortExitCode for special handling
 */
#define ABORT_CAST(err_code) static_cast<VMAbortExitCode>(err_code)

/**
 * Break the method and return VMAbortExitCode
 */
#define ABORT(err_code) return outcome::failure(ABORT_CAST(err_code))

namespace fc::vm {
  /**
   * specs-actors and custom exit code enum for outcome errors.
   */
  enum class VMExitCode : int64_t {
    kFatal = -1,

    // Old general actor error that is used for backward compatibility with old network versions
    kOldErrActorFailure = 1,

    kOk = 0,
    kSysErrSenderInvalid = 1,
    kSysErrSenderStateInvalid = 2,
    kSysErrInvalidMethod = 3,
    kSysErrReserved1 = 4,
    kSysErrInvalidReceiver = 5,
    kSysErrInsufficientFunds = 6,
    kSysErrOutOfGas = 7,
    kSysErrForbidden = 8,
    kSysErrIllegalActor = 9,
    kSysErrIllegalArgument = 10,
    kSysErrReserved2 = 11,
    kSysErrReserved3 = 12,
    kSysErrReserved4 = 13,
    kSysErrReserved5 = 14,
    kSysErrReserved6 = 15,

    kErrIllegalArgument = 16,
    kErrNotFound,
    kErrForbidden,
    kErrInsufficientFunds,
    kErrIllegalState,
    kErrSerialization,

    kErrFirstActorSpecificExitCode = 32,

    kErrPlaceholder = 1000,

    kEncodeActorResultError,

    kNotImplemented,
  };

  /**
   * Fatal VM error
   */
  enum class VMFatal : int64_t {
    kFatal = 1,
  };

  /**
   * VMExitCode that aborts execution and shouldn't be replaced
   */
  enum class VMAbortExitCode : int64_t {};

  /// Distinguish VMExitCode errors from other errors
  bool isVMExitCode(const std::error_code &error);

  /// Fatal VM error that should not be ignored
  bool isFatal(const std::error_code &error);

  /// VMExitCode that aborts execution
  bool isAbortExitCode(const std::error_code &error);

}  // namespace fc::vm

OUTCOME_HPP_DECLARE_ERROR(fc::vm, VMExitCode);

OUTCOME_HPP_DECLARE_ERROR(fc::vm, VMFatal);

OUTCOME_HPP_DECLARE_ERROR(fc::vm, VMAbortExitCode);

namespace fc::vm {
  /**
   * Aborts execution if res has error
   * @tparam T - result type
   * @param res - result to check
   * @param default_error - default VMExitCode to abort with.
   * @return If res has no error, success() returned. Otherwise if res.error()
   * is VMAbortExitCode or VMFatal, the res.error() returned, else
   * default_error returned
   */
  template <typename T>
  outcome::result<void> requireNoError(const outcome::result<T> &res,
                                       const VMExitCode &default_error) {
    if (res.has_error()) {
      if (isFatal(res.error()) || isAbortExitCode(res.error())) {
        return res.error();
      }
      if (isVMExitCode(res.error())) {
        ABORT(static_cast<VMExitCode>(res.error().value()));
      }
      ABORT(default_error);
    }
    return outcome::success();
  }

}  // namespace fc::vm
