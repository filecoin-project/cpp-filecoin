/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/storage_power/storage_power_actor_utils.hpp"

namespace fc::vm::actor::builtin::v0::storage_power {
  outcome::result<void> PowerUtils::validateMinerHasClaim(
      PowerActorStatePtr &state, const Address &miner) const {
    // Do nothing for v0
    return outcome::success();
  }
}  // namespace fc::vm::actor::builtin::v0::storage_power
