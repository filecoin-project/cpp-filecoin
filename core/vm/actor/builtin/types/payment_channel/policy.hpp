/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "const.hpp"
#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::types::payment_channel {
  constexpr size_t kLaneLimit{INT64_MAX};
  static EpochDuration kSettleDelay = fc::kEpochsInHour * 12;
  constexpr size_t kMaxSecretSize{256};

  inline void setPolicy(const EpochDuration &epochsInHour) {
    kSettleDelay = epochsInHour * 12;
  }
}  // namespace fc::vm::actor::builtin::types::payment_channel
