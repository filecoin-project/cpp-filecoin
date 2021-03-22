/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/payment_channel/policy.hpp"
#include "const.hpp"

namespace fc::vm::actor::builtin::types::payment_channel {
  EpochDuration kSettleDelay = kEpochsInHour * 12;

  void setPolicy(const EpochDuration &epochsInHour) {
    kSettleDelay = epochsInHour * 12;
  }
}
