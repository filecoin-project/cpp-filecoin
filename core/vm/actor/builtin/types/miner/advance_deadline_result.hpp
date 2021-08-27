/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/types.hpp"
#include "vm/actor/builtin/types/miner/power_pair.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using primitives::TokenAmount;

  struct AdvanceDeadlineResult {
    TokenAmount pledge_delta;
    PowerPair power_delta;
    PowerPair previously_faulty_power;
    PowerPair detected_faulty_power;
    PowerPair total_faulty_power;

    bool operator==(const AdvanceDeadlineResult &other) const {
      return pledge_delta == other.pledge_delta
             && power_delta == other.power_delta
             && previously_faulty_power == other.previously_faulty_power
             && detected_faulty_power == other.detected_faulty_power
             && total_faulty_power == other.total_faulty_power;
    }

    bool operator!=(const AdvanceDeadlineResult &other) const {
      return !(*this == other);
    }
  };

}  // namespace fc::vm::actor::builtin::types::miner
