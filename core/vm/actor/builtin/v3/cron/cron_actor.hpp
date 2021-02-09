/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/v2/cron/cron_actor.hpp"

namespace fc::vm::actor::builtin::v3::cron {

  /**
   * Cron actor v3 is identical to Cron actor v2
   */
  using State = v2::cron::State;
  using Construct = v2::cron::Construct;
  using EpochTick = v2::cron::EpochTick;

  extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::v3::cron
