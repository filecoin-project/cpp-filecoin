/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/cron_actor.hpp"

namespace fc::vm::actor {

  using vm::actor::MethodNumber;
  using vm::runtime::InvocationOutput;
  using vm::runtime::Runtime;

  constexpr MethodNumber kEpochTickMethodNumber{2};

  std::vector<CronTableEntry> CronActor::entries = {
      {kStoragePowerAddress, SpaMethods::CHECK_PROOF_SUBMISSIONS}};

  outcome::result<InvocationOutput> CronActor::epochTick(
      const Actor &actor,
      const std::shared_ptr<Runtime> &runtime,
      const MethodParams &params) {
    if (!(runtime->getMessage()->from == kCronAddress)) {
      return WRONG_CALL;
    }

    for (const auto &entry : entries) {
      OUTCOME_TRY(
          runtime->send(entry.to_addr, entry.method_num, {}, BigInt(0)));
    }
    return outcome::success();
  }

  ActorExports CronActor::exports = {
      {kEpochTickMethodNumber, ActorMethod(CronActor::epochTick)},
  };
}  // namespace fc::vm::actor
