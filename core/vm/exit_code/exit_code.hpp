/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/optional.hpp>
#include "common/outcome.hpp"
#include "vm/version/version.hpp"

/**
 * Aborts execution if res has error and aborts with default_error if res has
 * error and the error is not fatal or abort
 */
#define REQUIRE_NO_ERROR(expr, err_code) \
  OUTCOME_TRY(requireNoError((expr), (err_code)))

/**
 * In addition to REQUIRE_NO_ERROR allows to create res variable
 */
#define REQUIRE_NO_ERROR_A(res, expr, err_code) \
  OUTCOME_TRY((res), requireNoErrorAssign((expr), (err_code)))

/**
 * Aborts execution if res is not success
 */
#define REQUIRE_SUCCESS(expr) OUTCOME_TRY(requireSuccess((expr)))

/**
 * In addition to REQUIRE_SUCCESS allows to create res variable
 */
#define REQUIRE_SUCCESS_A(res, expr) \
  OUTCOME_TRY((res), requireSuccessAssign((expr)))

/**
 * Returns another error if res has error and the error is not fatal or
 * abort
 */
#define CHANGE_ERROR(expr, err_code) \
  OUTCOME_TRY(changeError((expr), (err_code)))

/**
 * In addition to CHANGE_ERROR allows to create res variable
 */
#define CHANGE_ERROR_A(res, expr, err_code) \
  OUTCOME_TRY((res), changeErrorAssign((expr), (err_code)))

/**
 * Aborts execution with error if res has error and the error is not fatal or
 * abort
 */
#define CHANGE_ERROR_ABORT(expr, err_code) \
  OUTCOME_TRY(changeErrorAbort((expr), (err_code)))

/**
 * In addition to CHANGE_ERROR_ABORT allows to create res variable
 */
#define CHANGE_ERROR_ABORT_A(res, expr, err_code) \
  OUTCOME_TRY((res), changeErrorAbortAssign((expr), (err_code)))

/**
 * Break the method and return VMAbortExitCode
 */
#define ABORT(err_code) return outcome::failure(asAbort(err_code))

#define VM_ASSERT(condition) OUTCOME_TRY(vm_assert(condition))

#define VALIDATE_ARG(condition) OUTCOME_TRY(validateArgument(condition))

#define REQUIRE_STATE(condition) OUTCOME_TRY(requireState(condition))

namespace fc::vm {
  /**
   * specs-actors and custom exit code enum for outcome errors.
   */
  enum class VMExitCode : int64_t {
    kAssert = -2,
    kFatal = -1,

    // Old general actor error that is used for backward compatibility with old
    // network versions
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

    kErrBalanceInvariantBroken = 1000,

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

  outcome::result<VMExitCode> asExitCode(const std::error_code &error);
  std::error_code catchAbort(const std::error_code &error,
                             version::NetworkVersion version);
}  // namespace fc::vm

OUTCOME_HPP_DECLARE_ERROR(fc::vm, VMExitCode);

OUTCOME_HPP_DECLARE_ERROR(fc::vm, VMFatal);

OUTCOME_HPP_DECLARE_ERROR(fc::vm, VMAbortExitCode);

namespace fc::vm {
  template <typename T>
  outcome::result<T> catchAbort(outcome::result<T> &&result,
                                version::NetworkVersion version) {
    if (!result) {
      return catchAbort(result.error(), version);
    }
    return result;
  }

  template <typename T>
  void catchAbort(outcome::result<T> &result, version::NetworkVersion version) {
    if (!result) {
      result = catchAbort(result.error(), version);
    }
  }

  template <typename T>
  outcome::result<VMExitCode> asExitCode(const outcome::result<T> &result) {
    if (result) {
      return outcome::success(VMExitCode::kOk);
    }
    return asExitCode(result.error());
  }

  /**
   * Return VMExitCode as VMAbortExitCode for special handling
   */
  inline VMAbortExitCode asAbort(VMExitCode code) {
    return static_cast<VMAbortExitCode>(code);
  }

  /**
   * Aborts execution if res has error
   * @tparam T - result type
   * @param res - result to check
   * @param default_error - default VMExitCode to abort with.
   * @return If res has no error, success() returned. Otherwise if res.error()
   * is VMAbortExitCode or VMFatal, the res.error() returned, else
   * if res.error()is VMExitCode, the aborted res.error()returned, else
   * aborted default_error returned
   */
  template <typename T>
  outcome::result<void> requireNoError(const outcome::result<T> &res,
                                       const VMExitCode &default_error) {
    if (res.has_error()) {
      if (isFatal(res.error()) || isAbortExitCode(res.error())) {
        return res.error();
      }
      if (const auto exit{asExitCode(res)}) {
        ABORT(exit.value());
      }
      ABORT(default_error);
    }
    return outcome::success();
  }

  /**
   * Aborts execution if res has error
   * @tparam T - result type
   * @param res - result to check
   * @param default_error - default VMExitCode to abort with.
   * @return If res has no error, res returned. Otherwise if res.error()
   * is VMAbortExitCode or VMFatal, the res.error() returned, else
   * if res.error()is VMExitCode, the aborted res.error()returned, else
   * aborted default_error returned
   */
  template <typename T>
  outcome::result<T> requireNoErrorAssign(outcome::result<T> &&res,
                                          const VMExitCode &default_error) {
    REQUIRE_NO_ERROR(res, default_error);
    return std::move(res);
  }

  /**
   * Aborts execution if res is not success
   * @tparam T - result type
   * @param res - result to check
   * @return If res has no error, success() returned. Otherwise VMAbortExitCode
   * returned
   */
  template <typename T>
  outcome::result<void> requireSuccess(const outcome::result<T> &res) {
    if (res.has_error()) {
      if (const auto exit{asExitCode(res)}) {
        ABORT(exit.value());
      }
      return res.error();
    }
    return outcome::success();
  }

  /**
   * Aborts execution if res is not success and returns it to assign
   * @tparam T - result type
   * @param res - result to check
   * @return If res has no error, res returned. Otherwise VMAbortExitCode
   * returned
   */
  template <typename T>
  outcome::result<T> requireSuccessAssign(outcome::result<T> &&res) {
    REQUIRE_SUCCESS(res);
    return std::move(res);
  }

  /**
   * Returns abort exit code if res has error
   * @tparam T - result type
   * @param res - result to check
   * @param error - VMExitCode to to return instead.
   * @return If res has no error, success() returned. Otherwise if res.error()
   * is VMAbortExitCode or VMFatal, the res.error() returned, else error
   * returned
   */
  template <typename T>
  outcome::result<void> changeError(const outcome::result<T> &res,
                                    const VMExitCode &error) {
    if (res.has_error()) {
      if (isFatal(res.error()) || isAbortExitCode(res.error())) {
        return res.error();
      }
      return outcome::failure(error);
    }
    return outcome::success();
  }

  /**
   * Returns abort exit code if res has error
   * @tparam T - result type
   * @param res - result to check
   * @param error - VMExitCode to to return instead.
   * @return If res has no error, res returned. Otherwise if res.error()
   * is VMAbortExitCode or VMFatal, the res.error() returned, else error
   * returned
   */
  template <typename T>
  outcome::result<T> changeErrorAssign(outcome::result<T> &&res,
                                       const VMExitCode &error) {
    CHANGE_ERROR(res, error);
    return std::move(res);
  }

  /**
   * Returns abort exit code if res has error
   * @tparam T - result type
   * @param res - result to check
   * @param error - VMExitCode to abort with.
   * @return If res has no error, success() returned. Otherwise if res.error()
   * is VMAbortExitCode or VMFatal, the res.error() returned, else error
   * returned
   */
  template <typename T>
  outcome::result<void> changeErrorAbort(const outcome::result<T> &res,
                                         const VMExitCode &error) {
    if (res.has_error()) {
      if (isFatal(res.error()) || isAbortExitCode(res.error())) {
        return res.error();
      }
      ABORT(error);
    }
    return outcome::success();
  }

  /**
   * Returns abort exit code if res has error
   * @tparam T - result type
   * @param res - result to check
   * @param error - VMExitCode to abort with.
   * @return If res has no error, res returned. Otherwise if res.error()
   * is VMAbortExitCode or VMFatal, the res.error() returned, else error
   * returned
   */
  template <typename T>
  outcome::result<T> changeErrorAbortAssign(outcome::result<T> &&res,
                                            const VMExitCode &error) {
    CHANGE_ERROR_ABORT(res, error);
    return std::move(res);
  }

  inline outcome::result<void> vm_assert(bool condition) {
    if (!condition) {
      ABORT(VMExitCode::kAssert);
    }
    return outcome::success();
  }

  inline outcome::result<void> validateArgument(bool condition) {
    if (!condition) {
      ABORT(VMExitCode::kErrIllegalArgument);
    }
    return outcome::success();
  }

  inline outcome::result<void> requireState(bool condition) {
    if (!condition) {
      ABORT(VMExitCode::kErrIllegalState);
    }
    return outcome::success();
  }

}  // namespace fc::vm
