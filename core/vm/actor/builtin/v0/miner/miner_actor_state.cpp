/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/miner/miner_actor_state.hpp"

namespace fc::vm::actor::builtin::v0::miner {

  DeadlineInfo State::deadlineInfo(ChainEpoch now) const {
    return DeadlineInfo(proving_period_start, current_deadline, now);
  }
}  // namespace fc::vm::actor::builtin::v0::miner
