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
      case E::Ok:
      case E::SysErrSenderInvalid:
      case E::SysErrSenderStateInvalid:
      case E::SysErrInvalidMethod:
      case E::SysErrInvalidParameters:
      case E::SysErrInvalidReceiver:
      case E::SysErrInsufficientFunds:
      case E::SysErrOutOfGas:
      case E::SysErrForbidden:
      case E::SysErrorIllegalActor:
      case E::SysErrorIllegalArgument:
      case E::SysErrSerialization:
      case E::SysErrorReserved3:
      case E::SysErrorReserved4:
      case E::SysErrorReserved5:
      case E::SysErrInternal:
        return error;

      case E::ErrIllegalArgument:
      case E::ErrNotFound:
      case E::ErrForbidden:
      case E::ErrInsufficientFunds:
      case E::ErrIllegalState:
      case E::ErrSerialization:
        return error;

      case E::ErrPlaceholder:
        return error;

      case E::DECODE_ACTOR_PARAMS_ERROR:
        return E{1};

      case E::ACCOUNT_ACTOR_CREATE_WRONG_ADDRESS_TYPE:
        return E::ErrIllegalArgument;
      case E::ACCOUNT_ACTOR_RESOLVE_NOT_FOUND:
      case E::ACCOUNT_ACTOR_RESOLVE_NOT_ACCOUNT_ACTOR:
        return E{1};

      case E::MINER_ACTOR_OWNER_NOT_SIGNABLE:
      case E::MINER_ACTOR_MINER_NOT_ACCOUNT:
      case E::MINER_ACTOR_MINER_NOT_BLS:
      case E::MINER_ACTOR_ILLEGAL_ARGUMENT:
        return E::ErrIllegalArgument;
      case E::MINER_ACTOR_NOT_FOUND:
        return E::ErrNotFound;
      case E::MINER_ACTOR_WRONG_CALLER:
      case E::MINER_ACTOR_WRONG_EPOCH:
        return E::ErrForbidden;
      case E::MINER_ACTOR_POST_TOO_LATE:
      case E::MINER_ACTOR_POST_TOO_EARLY:
      case E::MINER_ACTOR_INSUFFICIENT_FUNDS:
        return E::ErrInsufficientFunds;
      case E::MINER_ACTOR_ILLEGAL_STATE:
        return E::ErrIllegalState;

      case E::MARKET_ACTOR_ILLEGAL_ARGUMENT:
        return E::ErrIllegalArgument;
      case E::MARKET_ACTOR_WRONG_CALLER:
      case E::MARKET_ACTOR_FORBIDDEN:
        return E::ErrForbidden;
      case E::MARKET_ACTOR_INSUFFICIENT_FUNDS:
        return E::ErrInsufficientFunds;
      case E::MARKET_ACTOR_ILLEGAL_STATE:
        return E::ErrIllegalState;

      case E::MULTISIG_ACTOR_WRONG_CALLER:
        return E{1};
      case E::MULTISIG_ACTOR_ILLEGAL_ARGUMENT:
        return E::ErrIllegalArgument;
      case E::MULTISIG_ACTOR_NOT_FOUND:
        return E::ErrNotFound;
      case E::MULTISIG_ACTOR_FORBIDDEN:
        return E::ErrForbidden;
      case E::MULTISIG_ACTOR_INSUFFICIENT_FUNDS:
        return E::ErrInsufficientFunds;
      case E::MULTISIG_ACTOR_ILLEGAL_STATE:
        return E::ErrIllegalState;

      case E::PAYMENT_CHANNEL_WRONG_CALLER:
        return E{1};
      case E::PAYMENT_CHANNEL_ILLEGAL_ARGUMENT:
        return E::ErrIllegalArgument;
      case E::PAYMENT_CHANNEL_FORBIDDEN:
        return E::ErrForbidden;
      case E::PAYMENT_CHANNEL_ILLEGAL_STATE:
        return E::ErrIllegalState;

      // TODO(turuslan): FIL-128 StoragePowerActor
      case E::STORAGE_POWER_ACTOR_WRONG_CALLER:
      case E::STORAGE_POWER_ACTOR_OUT_OF_BOUND:
      case E::STORAGE_POWER_ACTOR_ALREADY_EXISTS:
      case E::STORAGE_POWER_DELETION_ERROR:
        break;
      case E::STORAGE_POWER_ILLEGAL_ARGUMENT:
        return E::ErrIllegalArgument;
      case E::STORAGE_POWER_FORBIDDEN:
        return E::ErrForbidden;
      case E::STORAGE_POWER_ILLEGAL_STATE:
        return E::ErrIllegalState;

      case E::INIT_ACTOR_NOT_BUILTIN_ACTOR:
      case E::INIT_ACTOR_SINGLETON_ACTOR:
        return E{1};

      case E::CRON_ACTOR_WRONG_CALL:
        return E{1};

      case E::REWARD_ACTOR_NEGATIVE_WITHDRAWABLE:
      case E::REWARD_ACTOR_WRONG_CALLER:
        return E{1};
    }
    return {};
  }
}  // namespace fc::vm
