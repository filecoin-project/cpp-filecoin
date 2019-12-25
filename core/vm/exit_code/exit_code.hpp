/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_EXITCODE_EXITCODE_HPP
#define CPP_FILECOIN_CORE_VM_EXITCODE_EXITCODE_HPP

#include <string>

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
