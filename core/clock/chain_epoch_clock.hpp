/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CLOCK_CHAIN_EPOCH_CLOCK_HPP
#define CPP_FILECOIN_CORE_CLOCK_CHAIN_EPOCH_CLOCK_HPP

#include "clock/time.hpp"
#include "primitives/chain_epoch.hpp"

namespace fc::clock {
  enum class EpochAtTimeError { BEFORE_GENESIS = 1 };

  using primitives::ChainEpoch;

  /**
   * Converts UTC time to epoch number
   */
  class ChainEpochClock {
   public:
    virtual Time genesisTime() const = 0;
    virtual outcome::result<ChainEpoch> epochAtTime(const Time &time) const = 0;
    virtual ~ChainEpochClock() = default;
  };

  constexpr std::chrono::nanoseconds kEpochDuration = std::chrono::seconds(15);
}  // namespace fc::clock

OUTCOME_HPP_DECLARE_ERROR(fc::clock, EpochAtTimeError);

#endif  // CPP_FILECOIN_CORE_CLOCK_CHAIN_EPOCH_CLOCK_HPP
