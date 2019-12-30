/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_CRON_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_CRON_ACTOR_HPP

#include "vm/actor/actor.hpp"
#include "vm/actor/storage_power_actor.hpp"
#include "vm/vm_context.hpp"

namespace fc::vm::actor {

  struct CronTableEntry {
    Address to_addr;
    uint64_t method_num;
  };

  struct CronActor {
    // Entries is a set of actors (and corresponding methods) to call during
    // EpochTick. This can be done a bunch of ways. We do it this way here to
    // make it easy to add a handler to Cron elsewhere in the spec code. How to
    // do this is implementation specific.
    static std::vector<CronTableEntry> entries;

    // EpochTick executes built-in periodic actions, run at every Epoch.
    // EpochTick(r) is called after all other messages in the epoch have been
    // applied. This can be seen as an implicit last message.
    /**
     * @brief EpochTick executes built-in periodic actions, run at every Epoch.
     * @param actor from Lotus(doesn't use)
     * @param vmctx is virtual machine context
     * @param params from Lotus(doesn't use)
     * @return success or error
     */
    static outcome::result<void> EpochTick(Actor &actor,
                                           vm::VMContext &vmctx,
                                           const std::vector<uint8_t> &params);
  };

}  // namespace fc::vm::actor

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_CRON_ACTOR_HPP
