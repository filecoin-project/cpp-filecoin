/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_EXITCODE_EXITCODE_HPP
#define CPP_FILECOIN_CORE_VM_EXITCODE_EXITCODE_HPP

#include <string>

#include "common/outcome.hpp"

namespace fc::vm {
  /**
   * VM exit code enum for outcome errors. Mapping to ret code in range 1-255 is
   * specified in `getRetCode`.
   */
  enum class VMExitCode {
    _ = 1,
    DECODE_ACTOR_PARAMS_ERROR,
    ENCODE_ACTOR_PARAMS_ERROR,

    INVOKER_NO_CODE_OR_METHOD,

    ACCOUNT_ACTOR_CREATE_WRONG_ADDRESS_TYPE,
    ACCOUNT_ACTOR_RESOLVE_NOT_FOUND,
    ACCOUNT_ACTOR_RESOLVE_NOT_ACCOUNT_ACTOR,

    MINER_ACTOR_OWNER_NOT_SIGNABLE,
    MINER_ACTOR_MINER_NOT_ACCOUNT,
    MINER_ACTOR_MINER_NOT_BLS,
    MINER_ACTOR_ILLEGAL_ARGUMENT,
    MINER_ACTOR_NOT_FOUND,
    MINER_ACTOR_WRONG_CALLER,
    MINER_ACTOR_WRONG_EPOCH,
    MINER_ACTOR_POST_TOO_LATE,
    MINER_ACTOR_POST_TOO_EARLY,
    MINER_ACTOR_INSUFFICIENT_FUNDS,
    MINER_ACTOR_ILLEGAL_STATE,

    MARKET_ACTOR_ILLEGAL_ARGUMENT,
    MARKET_ACTOR_WRONG_CALLER,
    MARKET_ACTOR_FORBIDDEN,
    MARKET_ACTOR_INSUFFICIENT_FUNDS,
    MARKET_ACTOR_ILLEGAL_STATE,

    MULTISIG_ACTOR_WRONG_CALLER,
    MULTISIG_ACTOR_ILLEGAL_ARGUMENT,
    MULTISIG_ACTOR_NOT_FOUND,
    MULTISIG_ACTOR_FORBIDDEN,
    MULTISIG_ACTOR_INSUFFICIENT_FUNDS,
    MULTISIG_ACTOR_ILLEGAL_STATE,

    PAYMENT_CHANNEL_WRONG_CALLER,
    PAYMENT_CHANNEL_ILLEGAL_ARGUMENT,
    PAYMENT_CHANNEL_FORBIDDEN,
    PAYMENT_CHANNEL_ILLEGAL_STATE,

    STORAGE_POWER_ACTOR_WRONG_CALLER,
    STORAGE_POWER_ACTOR_OUT_OF_BOUND,
    STORAGE_POWER_ACTOR_ALREADY_EXISTS,
    STORAGE_POWER_DELETION_ERROR,
    STORAGE_POWER_ILLEGAL_ARGUMENT,
    STORAGE_POWER_ILLEGAL_STATE,
    STORAGE_POWER_FORBIDDEN,

    INIT_ACTOR_NOT_BUILTIN_ACTOR,
    INIT_ACTOR_SINGLETON_ACTOR,

    CRON_ACTOR_WRONG_CALL,

    REWARD_ACTOR_NEGATIVE_WITHDRAWABLE,
    REWARD_ACTOR_WRONG_CALLER,

  };

  /// Distinguish VMExitCode errors from other errors
  bool isVMExitCode(const std::error_code &error);

  /// Get ret code from VMExitCode
  uint8_t getRetCode(VMExitCode error);

  /// Get ret code from VMExitCode error
  outcome::result<uint8_t> getRetCode(const std::error_code &error);

  /// Get ret code from outcome
  template <typename T>
  outcome::result<uint8_t> getRetCode(const outcome::result<T> &result) {
    if (result) {
      return 0;
    }
    return getRetCode(result.error());
  }
}  // namespace fc::vm

OUTCOME_HPP_DECLARE_ERROR(fc::vm, VMExitCode);

#endif  // CPP_FILECOIN_CORE_VM_EXITCODE_EXITCODE_HPP
