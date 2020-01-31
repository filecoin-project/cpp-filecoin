/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/cron_actor.hpp"

namespace fc::vm::actor::cron_actor {
  /**
   * Entries is a set of actors (and corresponding methods) to call during
   * EpochTick
   */
  std::vector<CronTableEntry> entries = {
      {kStoragePowerAddress, {SpaMethods::CHECK_PROOF_SUBMISSIONS}}};

  outcome::result<InvocationOutput> epochTick(const Actor &actor,
                                              Runtime &runtime,
                                              const MethodParams &params) {
    if ((runtime.getMessage().get().from != kCronAddress)) {
      return cron_actor::WRONG_CALL;
    }

    for (const auto &entry : entries) {
      OUTCOME_TRY(runtime.send(entry.to_addr, entry.method_num, {}, BigInt(0)));
    }
    return outcome::success();
  }

  ActorExports exports = {
      {kEpochTickMethodNumber, ActorMethod(epochTick)},
  };
}  // namespace fc::vm::actor::cron_actor
