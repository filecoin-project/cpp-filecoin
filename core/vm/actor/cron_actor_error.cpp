/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/cron_actor_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::vm::actor, CronActorError, e) {
  using fc::vm::actor::CronActorError;

  switch (e) {
    case (CronActorError::WRONG_CALL):
      return "CronActor: EpochTick is only callable as a part of tipset state "
             "computation";
    case (CronActorError::UNKNOWN):
      return "CronActor: unknown error";
  }
}
