/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/cron/cron_actor.hpp"
#include "vm/actor/builtin/storage_power/storage_power_actor_export.hpp"

namespace fc::vm::actor::builtin::cron {
  /**
   * Entries is a set of actors (and corresponding methods) to call during
   * EpochTick
   */
  std::vector<CronTableEntry> entries = {
      {kStoragePowerAddress, {storage_power::OnEpochTickEnd::Number}}};

  ACTOR_METHOD_IMPL(EpochTick) {
    if ((runtime.getMessage().get().from != kSystemActorAddress)) {
      return VMExitCode::CRON_ACTOR_WRONG_CALL;
    }

    for (const auto &entry : entries) {
      OUTCOME_TRY(runtime.send(entry.to_addr, entry.method_num, {}, BigInt(0)));
    }
    return outcome::success();
  }

  const ActorExports exports{
      exportMethod<EpochTick>(),
  };
}  // namespace fc::vm::actor::builtin::cron
