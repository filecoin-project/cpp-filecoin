/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_EXITCODE_EXITCODE_HPP
#define CPP_FILECOIN_CORE_VM_EXITCODE_EXITCODE_HPP

#include <boost/optional.hpp>

#include "common/outcome.hpp"

#define VM_ASSERT(condition)                                  \
  if (!(condition)) {                                         \
    return outcome::failure(ABORT_CAST(VMExitCode::kAssert)); \
  }

namespace fc::vm {
  /**
   * specs-actors and custom exit code enum for outcome errors.
   */
  enum class VMExitCode : int64_t {
    kFatal = -1,

    kErrSerializationPre7 = 1,

    kOk = 0,
    kSysErrSenderInvalid = 1,
    kSysErrSenderStateInvalid = 2,
    kSysErrInvalidMethod = 3,
    kSysErrInvalidParameters = 4,
    kSysErrInvalidReceiver = 5,
    kSysErrInsufficientFunds = 6,
    kSysErrOutOfGas = 7,
    kSysErrForbidden = 8,
    kSysErrorIllegalActor = 9,
    kSysErrorIllegalArgument = 10,
    kSysErrSerialization = 11,
    kSysErrorReserved3 = 12,
    kSysErrorReserved4 = 13,
    kSysErrorReserved5 = 14,
    kSysErrInternal = 15,

    kErrIllegalArgument,
    kErrNotFound,
    kErrForbidden,
    kErrInsufficientFunds,
    kErrIllegalState,
    kErrSerialization,

    kErrFirstActorSpecificExitCode = 32,

    kErrPlaceholder = 1000,

    kEncodeActorResultError,

    kSendTransferInsufficient,

    kAccountActorCreateWrongAddressType,
    kAccountActorResolveNotFound,
    kAccountActorResolveNotAccountActor,

    kMinerActorOwnerNotSignable,
    kMinerActorNotAccount,
    kMinerActorMinerNotBls,
    kMinerActorIllegalArgument,
    kMinerActorNotFound,
    kMinerActorWrongCaller,
    kMinerActorWrongEpoch,
    kMinerActorPostTooLate,
    kMinerActorPostTooEarly,
    kMinerActorInsufficientFunds,
    kMinerActorIllegalState,

    kInitActorNotBuiltinActor,
    kInitActorSingletonActor,

    kAssert,

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
}  // namespace fc::vm

OUTCOME_HPP_DECLARE_ERROR(fc::vm, VMExitCode);

OUTCOME_HPP_DECLARE_ERROR(fc::vm, VMFatal);

OUTCOME_HPP_DECLARE_ERROR(fc::vm, VMAbortExitCode);

namespace fc::vm {
  template <typename T>
  outcome::result<VMExitCode> asExitCode(const outcome::result<T> &result) {
    if (result) {
      return outcome::success(VMExitCode::kOk);
    }
    return asExitCode(result.error());
  }
}  // namespace fc::vm

#endif  // CPP_FILECOIN_CORE_VM_EXITCODE_EXITCODE_HPP
