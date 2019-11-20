/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clock/impl/chain_epoch_clock_impl.hpp"

namespace fc::clock {
  ChainEpochClockImpl::ChainEpochClockImpl(const Time &genesis_time)
      : genesis_time_(genesis_time) {}

  Time ChainEpochClockImpl::genesisTime() const {
    return genesis_time_;
  }

  outcome::result<ChainEpoch> ChainEpochClockImpl::epochAtTime(
      const Time &time) const {
    if (time < genesis_time_) {
      return EpochAtTimeError::BEFORE_GENESIS;
    }
    auto difference = time.unixTimeNano() - genesis_time_.unixTimeNano();
    return ChainEpoch(difference / kEpochDuration);
  }
}  // namespace fc::clock
