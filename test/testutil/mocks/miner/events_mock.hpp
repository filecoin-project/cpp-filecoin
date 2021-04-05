/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "miner/storage_fsm/events.hpp"

#include <gmock/gmock.h>

namespace fc::mining {
  class EventsMock : public Events {
   public:
    MOCK_METHOD4(chainAt,
                 outcome::result<void>(
                     HeightHandler, RevertHandler, EpochDuration, ChainEpoch));
  };
}  // namespace fc::mining
