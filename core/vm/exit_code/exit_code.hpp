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

    INVOKER_CANT_INVOKE_ACCOUNT_ACTOR,
    INVOKER_NO_CODE_OR_METHOD,

    ACCOUNT_ACTOR_CREATE_WRONG_ADDRESS_TYPE,
    ACCOUNT_ACTOR_RESOLVE_NOT_FOUND,
    ACCOUNT_ACTOR_RESOLVE_NOT_ACCOUNT_ACTOR,

    MULTISIG_ACTOR_WRONG_CALLER,
    MULTISIG_ACTOR_ILLEGAL_ARGUMENT,
    MULTISIG_ACTOR_NOT_FOUND,
    MULTISIG_ACTOR_FORBIDDEN,
    MULTISIG_ACTOR_INSUFFICIENT_FUNDS,
    MULTISIG_ACTOR_ILLEGAL_STATE,

    PAYMENT_CHANNEL_WRONG_CALLER,
    PAYMENT_CHANNEL_ILLEGAL_ARGUMENT,

    STORAGE_POWER_ACTOR_WRONG_CALLER,
    STORAGE_POWER_ACTOR_OUT_OF_BOUND,
    STORAGE_POWER_ACTOR_ALREADY_EXISTS,
    STORAGE_POWER_ACTOR_NOT_FOUND,
    STORAGE_POWER_DELETION_ERROR,

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

namespace fc::vm::exit_code {

  /**
   * @brief System error codes
   */
  enum class ErrorCode {

    /**
     * Successful exit code
     */
    kSuccess = 0,

    // System error codes

    /**
     * Represents a failure to find an actor
     */
    kActorNotFound,

    /**
     * Represents a failure to find the code for a particular actor in the VM
     * registry.
     */
    kActorCodeNotFound,

    /**
     * Represents a failure to find a method in an actor
     */
    kInvalidMethod,

    /**
     * Indicates that a method was called with the incorrect number of
     * arguments, or that its arguments did not satisfy its preconditions
     */
    kInvalidArgumentsSystem,

    /**
     * Represents a failure to apply a message, as it did not carry sufficient
     * funds
     */
    kInsufficientFundsSystem,

    /**
     * Represents a message invocation out of sequence. This happens when
     * message.CallSeqNum is not exactly actor.CallSeqNum + 1
     */
    kInvalidCallSeqNum,

    /**
     * Returned when the execution of an actor method (including its subcalls)
     * uses more gas than initially allocated
     */
    kOutOfGas,

    /**
     * Returned when an actor method invocation makes a call to the runtime that
     * does not satisfy its preconditions
     */
    kRuntimeAPIError,

    /**
     * Returned when an actor method invocation calls rt.Assert with a false
     * condition
     */
    kRuntimeAssertFailure,

    /**
     * Returned when an actor method's Send call has returned with a failure
     * error code (and the Send call did not specify to ignore errors)
     */
    kMethodSubcallError,

    // User defined error codes

    kInsufficientFundsUser,
    kInvalidArgumentsUser,
    kInconsistentStateUser,

    kInvalidSectorPacking,
    kSealVerificationFailed,
    kPoStVerificationFailed,
    kDeadlineExceeded,
    kInsufficientPledgeCollateral,
  };

  /**
   * @brief Virtual machine exit code
   */
  class ExitCode {
   public:
    /**
     * @brief Create exit code
     * @param exit_code - code to set
     */
    explicit ExitCode(ErrorCode error_code);

    /**
     * @brief Create successful exit code
     * @return Successful exit code
     */
    static ExitCode makeOkExitCode();

    /**
     * @brief Create error exit code
     * @param exit_code - code to set
     */
    static ExitCode makeErrorExitCode(ErrorCode error_code);

    /**
     * @brief ensure if exit code is error code
     * @param exit_code - to check
     * @return exit_code if error code is present or ExitCode with
     * SystemError::kRuntimeAPIError set otherwise
     */
    static ExitCode ensureErrorCode(const ExitCode &exit_code);

    /**
     * @brief Check if exit code is Success
     * @return true if no error
     */
    bool isSuccess() const;

    /**
     * @brief Check if exit code is System or User defined error
     * @return true if error
     */
    bool isError() const;

    /**
     * @brief Check if state update is allowed
     * @return true if is successful
     */
    bool allowsStateUpdate() const;

    std::string toString() const;

    bool operator==(const ExitCode &other) const;

   private:
    ErrorCode exit_code_;
  };

  /**
   * @brief Runtime error with text message
   */
  class RuntimeError {
   public:
    RuntimeError(ExitCode exit_code, std::string error_message);
    explicit RuntimeError(ExitCode exit_code);

    std::string toString() const;

   private:
    ExitCode exit_code_;
    std::string error_message_;
  };

}  // namespace fc::vm::exit_code

#endif  // CPP_FILECOIN_CORE_VM_EXITCODE_EXITCODE_HPP
