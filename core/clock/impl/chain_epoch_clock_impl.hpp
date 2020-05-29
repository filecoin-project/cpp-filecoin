/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CLOCK_IMPL_CHAIN_EPOCH_CLOCK_IMPL_HPP
#define CPP_FILECOIN_CORE_CLOCK_IMPL_CHAIN_EPOCH_CLOCK_IMPL_HPP

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

#endif  // CPP_FILECOIN_CORE_CLOCK_IMPL_CHAIN_EPOCH_CLOCK_IMPL_HPP
