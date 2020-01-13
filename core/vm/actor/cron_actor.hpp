/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_CRON_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_CRON_ACTOR_HPP

#include "vm/actor/actor_method.hpp"
#include "vm/actor/storage_power_actor.hpp"

namespace fc::vm::actor {

  struct CronTableEntry {
    Address to_addr;
    uint64_t method_num;
  };

  struct CronActor {
    static constexpr VMExitCode WRONG_CALL{1};

    /**
     * Entries is a set of actors (and corresponding methods) to call during
     * EpochTick
     */
    static std::vector<CronTableEntry> entries;

    /**
     * @brief EpochTick executes built-in periodic actions, run at every Epoch.
     * @param actor from Lotus(doesn't use)
     * @param vmctx is virtual machine context
     * @param params from Lotus(doesn't use)
     * @return success or error
     */
    static outcome::result<Buffer> epochTick(const Actor &actor,
                                             VMContext &vmctx,
                                             gsl::span<const uint8_t> params);
  };

}  // namespace fc::vm::actor

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_CRON_ACTOR_HPP
