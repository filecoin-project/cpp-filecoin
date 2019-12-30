/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_CRON_ACTOR_ERROR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_CRON_ACTOR_ERROR_HPP

#include "common/outcome.hpp"

namespace fc::vm::actor {

  /**
   * @brief CronActor returns these types of errors
   */
  enum class CronActorError {
    WRONG_CALL = 1,

    UNKNOWN = 1000
  };

}  // namespace fc::vm::actor

OUTCOME_HPP_DECLARE_ERROR(fc::vm::actor, CronActorError);

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_CRON_ACTOR_ERROR_HPP
