/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/types.hpp"

namespace fc {
  using primitives::ActorId;
  using primitives::ChainEpoch;

  enum class ConsensusFaultType : int64_t {
    DoubleForkMining = 1,
    ParentGrinding = 2,
    TimeOffsetMining = 3,
  };

  struct ConsensusFault {
    ActorId target{};
    ChainEpoch epoch{};
    ConsensusFaultType type{};
  };
}  // namespace fc
