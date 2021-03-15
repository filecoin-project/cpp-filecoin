/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/states/state.hpp"

#include "primitives/address/address.hpp"
#include "vm/actor/builtin/types/cron/cron_table_entry.hpp"

namespace fc::vm::actor::builtin::states {
  using types::cron::CronTableEntry;

  struct CronActorState : State {
    std::vector<CronTableEntry> entries;
  };

  using CronActorStatePtr = std::shared_ptr<CronActorState>;
}  // namespace fc::vm::actor::builtin::states
