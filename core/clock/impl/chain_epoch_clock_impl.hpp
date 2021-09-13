/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "clock/chain_epoch_clock.hpp"

namespace fc::clock {
  class ChainEpochClockImpl : public ChainEpochClock {
   public:
    explicit ChainEpochClockImpl(UnixTime genesis_time);
    UnixTime genesisTime() const override;
    outcome::result<ChainEpoch> epochAtTime(UnixTime time) const override;

   private:
    UnixTime genesis_time_;
  };
}  // namespace fc::clock
