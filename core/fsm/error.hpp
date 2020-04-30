/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_FSM_ERROR_HPP
#define CPP_FILECOIN_CORE_FSM_ERROR_HPP

#include "common/outcome.hpp"

namespace fc::fsm {

  enum class FsmError {
    ENTITY_NOT_TRACKED = 1,
    ENTITY_ALREADY_BEING_TRACKED,
    MACHINE_STOPPED,
  };

}

OUTCOME_HPP_DECLARE_ERROR(fc::fsm, FsmError);

#endif  // CPP_FILECOIN_CORE_FSM_ERROR_HPP
