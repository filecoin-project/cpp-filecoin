/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "primitives/address/address.hpp"
#include "vm/actor/builtin/states/storage_power/storage_power_actor_state.hpp"
#include "vm/actor/builtin/utils/actor_utils.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::utils {
  using primitives::address::Address;
  using runtime::Runtime;
  using states::PowerActorStatePtr;

  class PowerUtils : public ActorUtils {
   public:
    explicit PowerUtils(Runtime &r) : ActorUtils(r) {}

    virtual outcome::result<void> validateMinerHasClaim(
        PowerActorStatePtr &state, const Address &miner) const = 0;
  };

  using PowerUtilsPtr = std::shared_ptr<PowerUtils>;

}  // namespace fc::vm::actor::builtin::utils
