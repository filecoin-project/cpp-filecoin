/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_CRON_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_CRON_ACTOR_HPP

#include "vm/actor/actor_method.hpp"

namespace fc::vm::actor::builtin::cron {

  struct CronTableEntry {
    Address to_addr;
    MethodNumber method_num{};
  };
  CBOR_TUPLE(CronTableEntry, to_addr, method_num)

  struct State {
    std::vector<CronTableEntry> entries;
  };
  CBOR_TUPLE(State, entries)

  /**
   * @brief EpochTick executes built-in periodic actions, run at every Epoch.
   */
  struct EpochTick : ActorMethodBase<2> {
    ACTOR_METHOD_DECL();
  };

  extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::cron

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_CRON_ACTOR_HPP
