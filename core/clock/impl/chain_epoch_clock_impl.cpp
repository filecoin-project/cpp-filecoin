/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clock/impl/chain_epoch_clock_impl.hpp"

namespace fc::clock {
  ChainEpochClockImpl::ChainEpochClockImpl(UnixTime genesis_time)
      : genesis_time_(genesis_time) {}

  UnixTime ChainEpochClockImpl::genesisTime() const {
    return genesis_time_;
  }

  outcome::result<ChainEpoch> ChainEpochClockImpl::epochAtTime(
      UnixTime time) const {
    if (time < genesis_time_) {
      return outcome::failure(EpochAtTimeError::kBeforeGenesis);
    }
    auto difference = time - genesis_time_;
    return ChainEpoch(difference / kEpochDuration);
  }
}  // namespace fc::clock
