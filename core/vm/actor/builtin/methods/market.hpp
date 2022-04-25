/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"

namespace fc::vm::actor::builtin {

  // These methods must be actual with the last version of actors

  enum class MarketActor : MethodNumber {
    kConstruct = 1,
    kAddBalance,
    kWithdrawBalance,
    kPublishStorageDeals,
    kVerifyDealsForActivation,
    kActivateDeals,
    kOnMinerSectorsTerminate,
    kComputeDataCommitment,
    kCronTick,
  }

}  // namespace fc::vm::actor::builtin
