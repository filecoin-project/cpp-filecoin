/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/cron_actor.hpp"

namespace fc::vm::actor {
  std::vector<CronTableEntry> CronActor::entries = {
      {kStoragePowerAddress, SpaMethods::CHECK_PROOF_SUBMISSIONS}};

  outcome::result<Buffer> CronActor::epochTick(
      const Actor &actor,
      vm::VMContext &vmctx,
      gsl::span<const uint8_t> params) {
    if (!(vmctx.message().from == kCronAddress)) {
      return VMExitCode(1);
    }

    for (const auto &entry : entries) {
      OUTCOME_TRY(vmctx.send(entry.to_addr, entry.method_num, BigInt(0), {}));
    }
    return outcome::success();
  }
}  // namespace fc::vm::actor
