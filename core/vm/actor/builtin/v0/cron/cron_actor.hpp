/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/states/cron_actor_state.hpp"

namespace fc::vm::actor::builtin::v0::cron {

  struct Construct : ActorMethodBase<1> {
    using Params = std::vector<states::cron::CronTableEntry>;
    ACTOR_METHOD_DECL();
  };

  /**
   * @brief EpochTick executes built-in periodic actions, run at every Epoch.
   */
  struct EpochTick : ActorMethodBase<2> {
    ACTOR_METHOD_DECL();
  };

  extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::v0::cron
