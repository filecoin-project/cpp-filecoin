/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/market/policy.hpp"
#include "const.hpp"

namespace fc::vm::actor::builtin::types::market {
  EpochDuration kDealUpdatesInterval = kEpochsInDay;

  void setPolicy(size_t epochsInDay) {
    kDealUpdatesInterval = epochsInDay;
  }

}
