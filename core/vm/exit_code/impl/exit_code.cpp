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

      case E::INVOKER_NO_CODE_OR_METHOD:
        return 255;

      case E::ACCOUNT_ACTOR_CREATE_WRONG_ADDRESS_TYPE:
      case E::ACCOUNT_ACTOR_RESOLVE_NOT_FOUND:
      case E::ACCOUNT_ACTOR_RESOLVE_NOT_ACCOUNT_ACTOR:
        return 1;

      case E::MINER_ACTOR_OWNER_NOT_SIGNABLE:
      case E::MINER_ACTOR_MINER_NOT_ACCOUNT:
      case E::MINER_ACTOR_MINER_NOT_BLS:
      case E::MINER_ACTOR_ILLEGAL_ARGUMENT:
        return 16;  // ErrIllegalArgument in actor-specs
      case E::MINER_ACTOR_NOT_FOUND:
        return 17;  // ErrNotFound in actor-specs
      case E::MINER_ACTOR_WRONG_CALLER:
      case E::MINER_ACTOR_WRONG_EPOCH:
        return 18;  // ErrForbidden in actor-specs
      case E::MINER_ACTOR_POST_TOO_LATE:
      case E::MINER_ACTOR_POST_TOO_EARLY:
      case E::MINER_ACTOR_INSUFFICIENT_FUNDS:
        return 19;  // ErrInsufficientFunds in actor-specs
      case E::MINER_ACTOR_ILLEGAL_STATE:
        return 20;  // ErrIllegalState in actor-specs

      case E::MARKET_ACTOR_ILLEGAL_ARGUMENT:
        return 16;  // ErrIllegalArgument in actor-specs
      case E::MARKET_ACTOR_WRONG_CALLER:
      case E::MARKET_ACTOR_FORBIDDEN:
        return 18;  // ErrForbidden in actor-specs
      case E::MARKET_ACTOR_INSUFFICIENT_FUNDS:
        return 19;  // ErrInsufficientFunds in actor-specs
      case E::MARKET_ACTOR_ILLEGAL_STATE:
        return 20;  // ErrIllegalState in actor-specs

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
      case E::PAYMENT_CHANNEL_FORBIDDEN:
        return 18;  // ErrForbidden in actor-specs
      case E::PAYMENT_CHANNEL_ILLEGAL_STATE:
        return 20;  // ErrIllegalState in actor-specs

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
      case E::STORAGE_POWER_ILLEGAL_STATE:
        return 20;  // ErrIllegalState in actor-specs

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
    // This should never be executed
    return 0;
  }

  outcome::result<uint8_t> getRetCode(const std::error_code &error) {
    if (!isVMExitCode(error)) {
      return error;
    }
    return getRetCode(VMExitCode(error.value()));
  }
}  // namespace fc::vm
