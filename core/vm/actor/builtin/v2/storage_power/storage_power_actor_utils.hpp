/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/utils/power_actor_utils.hpp"

namespace fc::vm::actor::builtin::v2::storage_power {
  using primitives::address::Address;
  using runtime::Runtime;
  using states::PowerActorStatePtr;

  class PowerUtils : public utils::PowerUtils {
   public:
    explicit PowerUtils(Runtime &r) : utils::PowerUtils(r) {}

    outcome::result<void> validateMinerHasClaim(
        PowerActorStatePtr &state, const Address &miner) const override;
  };
}  // namespace fc::vm::actor::builtin::v2::storage_power
