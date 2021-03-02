/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "primitives/address/address.hpp"
#include "vm/actor/builtin/states/power_actor_state.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::utils {
  using primitives::address::Address;
  using runtime::Runtime;

  class PowerUtils {
   public:
    explicit PowerUtils(Runtime &r) : runtime(r) {}
    virtual ~PowerUtils() = default;

    virtual outcome::result<void> validateMinerHasClaim(
        states::PowerActorStatePtr state, const Address &miner) const = 0;

   protected:
    Runtime &runtime;
  };

  using PowerUtilsPtr = std::shared_ptr<PowerUtils>;

}  // namespace fc::vm::actor::builtin::utils
