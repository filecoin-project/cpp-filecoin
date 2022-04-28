/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"

#include "vm/actor/builtin/types/cron/cron_table_entry.hpp"

namespace fc::vm::actor::builtin::cron {
  using types::cron::CronTableEntry;

  // These methods must be actual with the last version of actors

  enum class CronActor : MethodNumber {
    kConstruct = 1,
    kEpochTick,
  };

  struct Construct : ActorMethodBase<MethodNumber(CronActor::kConstruct)> {
    using Params = std::vector<CronTableEntry>;
  };

  struct EpochTick : ActorMethodBase<MethodNumber(CronActor::kEpochTick)> {};

}  // namespace fc::vm::actor::builtin::cron
