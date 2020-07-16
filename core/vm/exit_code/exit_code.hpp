/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_EXITCODE_EXITCODE_HPP
#define CPP_FILECOIN_CORE_VM_EXITCODE_EXITCODE_HPP

#include <boost/optional.hpp>

#include "common/outcome.hpp"

#define VM_ASSERT(condition)                      \
  if (!(condition)) {                             \
    return outcome::failure(VMExitCode::kAssert); \
  }

namespace fc::vm {
  /**
   * specs-actors and custom exit code enum for outcome errors.
   */
  enum class VMExitCode : int64_t {
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

    kErrPlaceholder = 1000,

    kDecodeActorParamsError,
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

    kMarketActorIllegalArgument,
    kMarketActorWrongCaller,
    kMarketActorForbidden,
    kMarketActorInsufficientFunds,
    kMarketActorIllegalState,

    kMultisigActorWrongCaller,
    kMultisigActorIllegalArgument,
    kMultisigActorNotFound,
    kMultisigActorForbidden,
    kMultisigActorInsufficientFunds,
    kMultisigActorIllegalState,

    kPaymentChannelWrongCaller,
    kPaymentChannelIllegalArgument,
    kPaymentChannelForbidden,
    kPaymentChannelIllegalState,

    kStoragePowerActorWrongCaller,
    kStoragePowerActorOutOfBound,
    kStoragePowerActorAlreadyExists,
    kStoragePowerActorDeletionError,
    kStoragePowerActorIllegalArgument,
    kStoragePowerActorIllegalState,
    kStoragePowerActorForbidden,

    kInitActorNotBuiltinActor,
    kInitActorSingletonActor,

    kRewardActorNegativeWithdrawable,
    kRewardActorWrongCaller,

    kAssert,
  };

  /// Distinguish VMExitCode errors from other errors
  bool isVMExitCode(const std::error_code &error);

  /// Get specs-actors VMExitCode
  boost::optional<VMExitCode> normalizeVMExitCode(VMExitCode error);
}  // namespace fc::vm

OUTCOME_HPP_DECLARE_ERROR(fc::vm, VMExitCode);

#endif  // CPP_FILECOIN_CORE_VM_EXITCODE_EXITCODE_HPP
