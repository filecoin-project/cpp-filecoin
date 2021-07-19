/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/address/address.hpp"
#include "vm/actor/builtin/types/cron/cron_table_entry.hpp"
#include "vm/actor/builtin/types/type_manager/universal.hpp"

namespace fc::vm::actor::builtin::states {
  using types::cron::CronTableEntry;

  struct CronActorState {
    virtual ~CronActorState() = default;

    std::vector<CronTableEntry> entries;
  };

  using CronActorStatePtr = types::Universal<CronActorState>;

}  // namespace fc::vm::actor::builtin::states
