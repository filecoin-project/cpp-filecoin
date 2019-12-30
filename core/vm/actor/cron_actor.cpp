/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/cron_actor.hpp"
#include "vm/actor/cron_actor_error.hpp"

std::vector<fc::vm::actor::CronTableEntry> fc::vm::actor::CronActor::entries = {
    {kStoragePowerAddress, SpaMethods::CHECK_PROOF_SUBMISSIONS}};

fc::outcome::result<void> fc::vm::actor::CronActor::EpochTick(
    fc::vm::actor::Actor &actor,
    fc::vm::VMContext &vmctx,
    const std::vector<uint8_t> &params) {
  if (!(vmctx.message().from == kCronAddress)) {
    return CronActorError::WRONG_CALL;
  }

  for (const auto& entry : entries) {
    OUTCOME_TRY(vmctx.send(entry.to_addr, entry.method_num, BigInt(1), {}));
  }
  return outcome::success();
}
