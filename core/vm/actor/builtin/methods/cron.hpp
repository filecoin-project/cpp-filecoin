/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"

#include "vm/actor/builtin/types/cron/cron_table_entry.hpp"

namespace fc::vm::actor::builtin {
  using types::cron::CronTableEntry;

  // These methods must be actual with the last version of actors

  struct CronActor {
    enum class Method : MethodNumber {
      kConstruct = 1,
      kEpochTick,
    }

    struct Construct : ActorMethodBase<Method::kConstruct> {
      using Params = std::vector<CronTableEntry>;
    };

    struct EpochTick : ActorMethodBase<Method::kEpochTick> {};
  };

}  // namespace fc::vm::actor::builtin
