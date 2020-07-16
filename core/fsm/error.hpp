/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_FSM_ERROR_HPP
#define CPP_FILECOIN_CORE_FSM_ERROR_HPP

#include "common/outcome.hpp"

namespace fc::fsm {

  enum class FsmError {
    kEntityNotTracked = 1,
    kEntityAlreadyBeingTracked,
    kMachineStopped,
  };

}

OUTCOME_HPP_DECLARE_ERROR(fc::fsm, FsmError);

#endif  // CPP_FILECOIN_CORE_FSM_ERROR_HPP
