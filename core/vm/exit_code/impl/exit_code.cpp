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

  boost::optional<VMExitCode> normalizeVMExitCode(VMExitCode error) {
    using E = VMExitCode;
    switch (error) {
      case E::kOk:
      case E::kSysErrSenderInvalid:
      case E::kSysErrSenderStateInvalid:
      case E::kSysErrInvalidMethod:
      case E::kSysErrInvalidParameters:
      case E::kSysErrInvalidReceiver:
      case E::kSysErrInsufficientFunds:
      case E::kSysErrOutOfGas:
      case E::kSysErrForbidden:
      case E::kSysErrorIllegalActor:
      case E::kSysErrorIllegalArgument:
      case E::kSysErrSerialization:
      case E::kSysErrorReserved3:
      case E::kSysErrorReserved4:
      case E::kSysErrorReserved5:
      case E::kSysErrInternal:
        return error;

      case E::kErrIllegalArgument:
      case E::kErrNotFound:
      case E::kErrForbidden:
      case E::kErrInsufficientFunds:
      case E::kErrIllegalState:
      case E::kErrSerialization:
        return error;

      case E::kErrPlaceholder:
        return error;

      case E::kDecodeActorParamsError:
        return E{1};
      case E::kEncodeActorResultError:
        return E{2};

      case E::kSendTransferInsufficient:
        return E{1};

      case E::kAccountActorCreateWrongAddressType:
        return E::kErrIllegalArgument;
      case E::kAccountActorResolveNotFound:
      case E::kAccountActorResolveNotAccountActor:
        return E{1};

      case E::kMinerActorOwnerNotSignable:
      case E::kMinerActorNotAccount:
      case E::kMinerActorMinerNotBls:
      case E::kMinerActorIllegalArgument:
        return E::kErrIllegalArgument;
      case E::kMinerActorNotFound:
        return E::kErrNotFound;
      case E::kMinerActorWrongCaller:
      case E::kMinerActorWrongEpoch:
        return E::kErrForbidden;
      case E::kMinerActorPostTooLate:
      case E::kMinerActorPostTooEarly:
      case E::kMinerActorInsufficientFunds:
        return E::kErrInsufficientFunds;
      case E::kMinerActorIllegalState:
        return E::kErrIllegalState;

      case E::kMarketActorIllegalArgument:
        return E::kErrIllegalArgument;
      case E::kMarketActorWrongCaller:
      case E::kMarketActorForbidden:
        return E::kErrForbidden;
      case E::kMarketActorInsufficientFunds:
        return E::kErrInsufficientFunds;
      case E::kMarketActorIllegalState:
        return E::kErrIllegalState;

      case E::kMultisigActorWrongCaller:
        return E{1};
      case E::kMultisigActorIllegalArgument:
        return E::kErrIllegalArgument;
      case E::kMultisigActorNotFound:
        return E::kErrNotFound;
      case E::kMultisigActorForbidden:
        return E::kErrForbidden;
      case E::kMultisigActorInsufficientFunds:
        return E::kErrInsufficientFunds;
      case E::kMultisigActorIllegalState:
        return E::kErrIllegalState;

      case E::kPaymentChannelWrongCaller:
        return E{1};
      case E::kPaymentChannelIllegalArgument:
        return E::kErrIllegalArgument;
      case E::kPaymentChannelForbidden:
        return E::kErrForbidden;
      case E::kPaymentChannelIllegalState:
        return E::kErrIllegalState;

      // TODO(turuslan): FIL-128 StoragePowerActor
      case E::kStoragePowerActorWrongCaller:
      case E::kStoragePowerActorOutOfBound:
      case E::kStoragePowerActorAlreadyExists:
      case E::kStoragePowerActorDeletionError:
        break;
      case E::kStoragePowerActorIllegalArgument:
        return E::kErrIllegalArgument;
      case E::kStoragePowerActorForbidden:
        return E::kErrForbidden;
      case E::kStoragePowerActorIllegalState:
        return E::kErrIllegalState;

      case E::kInitActorNotBuiltinActor:
      case E::kInitActorSingletonActor:
        return E{1};

      case E::kRewardActorNegativeWithdrawable:
      case E::kRewardActorWrongCaller:
        return E{1};

      case E::kAssert:
        return E{1};
    }
    return {};
  }
}  // namespace fc::vm
