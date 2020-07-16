/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CLOCK_CHAIN_EPOCH_CLOCK_HPP
#define CPP_FILECOIN_CORE_CLOCK_CHAIN_EPOCH_CLOCK_HPP

#include "clock/time.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"

namespace fc::clock {
  enum class EpochAtTimeError { kBeforeGenesis = 1 };

  using primitives::ChainEpoch;

  /**
   * Converts UTC time to epoch number
   */
  class ChainEpochClock {
   public:
    virtual UnixTime genesisTime() const = 0;
    virtual outcome::result<ChainEpoch> epochAtTime(UnixTime time) const = 0;
    virtual ~ChainEpochClock() = default;
  };

  constexpr UnixTime kEpochDuration{15};
}  // namespace fc::clock

OUTCOME_HPP_DECLARE_ERROR(fc::clock, EpochAtTimeError);

#endif  // CPP_FILECOIN_CORE_CLOCK_CHAIN_EPOCH_CLOCK_HPP
