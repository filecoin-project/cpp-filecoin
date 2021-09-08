/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/types/miner/power_pair.hpp"

namespace fc::vm::actor::builtin::types::miner {

  struct PoStResult {
    PowerPair power_delta;
    PowerPair new_faulty_power;
    PowerPair retracted_recovery_power;
    PowerPair recovered_power;
    RleBitset sectors;
    RleBitset ignored_sectors;
    RleBitset partitions;

    PowerPair powerDelta() const {
      return recovered_power - new_faulty_power;
    }

    PowerPair penaltyPower() const {
      return new_faulty_power + retracted_recovery_power;
    }
  };

}  // namespace fc::vm::actor::builtin::types::miner
