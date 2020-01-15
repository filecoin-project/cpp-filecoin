/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_VM_RUNTIME_RUNTIM_ERROR
#define FILECOIN_CORE_VM_RUNTIME_RUNTIM_ERROR

#include "common/outcome.hpp"

namespace fc::vm::runtime {

  /**
   * @brief Type of errors returned by Runtime
   */
  enum class RuntimeError {
    WRONG_ADDRESS_PROTOCOL = 1,
    CREATE_ACTOR_OPERATION_NOT_PERMITTED,
    DELETE_ACTOR_OPERATION_NOT_PERMITTED,
    ACTOR_NOT_FOUND,
    UNKNOWN = 1000
  };

}  // namespace fc::vm::runtime

OUTCOME_HPP_DECLARE_ERROR(fc::vm::runtime, RuntimeError);

#endif  // FILECOIN_CORE_VM_RUNTIME_RUNTIM_ERROR
