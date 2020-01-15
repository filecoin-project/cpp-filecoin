/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/cron_actor.hpp"

namespace fc::vm::actor {

  using vm::actor::MethodNumber;
  using vm::runtime::Runtime;

  std::vector<CronTableEntry> CronActor::entries = {
      {kStoragePowerAddress, SpaMethods::CHECK_PROOF_SUBMISSIONS}};

  outcome::result<Buffer> CronActor::epochTick(
      const Actor &actor,
      const std::shared_ptr<Runtime> &runtime,
      gsl::span<const uint8_t> params) {
    if (!(runtime->getMessage()->from == kCronAddress)) {
      return WRONG_CALL;
    }

    for (const auto &entry : entries) {
      OUTCOME_TRY(runtime->send(
          entry.to_addr, MethodNumber{entry.method_num}, {}, BigInt(0)));
    }
    return outcome::success();
  }

  ActorExports CronActor::exports = {
      {2, ActorMethod(CronActor::epochTick)},
  };
}  // namespace fc::vm::actor
