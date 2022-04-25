/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"

namespace fc::vm::actor::builtin {

  // These methods must be actual with the last version of actors

  enum class PowerActor : MethodNumber {
    kConstruct = 1,
    kCreateMiner,
    kUpdateClaimedPower,
    kEnrollCronEvent,
    kCronTick,  // since v7, OnEpochTickEnd for v6 and early
    kUpdatePledgeTotal,
    kOnConsensusFault,  // deprecated since v2
    kSubmitPoRepForBulkVerify,
    kCurrentTotalPower,
  }

}  // namespace fc::vm::actor::builtin
