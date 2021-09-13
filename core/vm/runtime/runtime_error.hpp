/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"

namespace fc::vm::runtime {

  /**
   * @brief Type of errors returned by Runtime
   */
  enum class RuntimeError {
    kWrongAddressProtocol = 1,
    kCreateActorOperationNotPermitted,
    kDeleteActorOperationNotPermitted,
    kActorNotFound,
    kNotEnoughFunds,
    kNotEnoughGas,
    kUnknown = 1000
  };

}  // namespace fc::vm::runtime

OUTCOME_HPP_DECLARE_ERROR(fc::vm::runtime, RuntimeError);
