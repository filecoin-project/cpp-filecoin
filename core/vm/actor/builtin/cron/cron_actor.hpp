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

  constexpr MethodNumber kEpochTickMethodNumber{2};

  /**
   * @brief EpochTick executes built-in periodic actions, run at every Epoch.
   * @param actor from Lotus(doesn't use)
   * @param runtime is virtual machine context
   * @param params from Lotus(doesn't use)
   * @return success or error
   */
  outcome::result<InvocationOutput> epochTick(const Actor &actor,
                                              Runtime &runtime,
                                              const MethodParams &params);

  extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::cron

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_CRON_ACTOR_HPP
