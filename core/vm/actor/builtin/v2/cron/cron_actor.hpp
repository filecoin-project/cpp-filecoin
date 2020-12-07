/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_BUILTEIN_V2_CRON_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_BUILTEIN_V2_CRON_ACTOR_HPP

#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/v0/cron/cron_actor.hpp"

namespace fc::vm::actor::builtin::v2::cron {

  /**
   * Cron actor v2 is identical to Cron actor v0
   */
  using Construct = v0::cron::Construct;
  using EpochTick = v0::cron::EpochTick;

  extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::v2::cron

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_CRON_ACTOR_HPP
