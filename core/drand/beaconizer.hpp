/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_DRAND_BEACONIZER_HPP
#define CPP_FILECOIN_CORE_DRAND_BEACONIZER_HPP

#include "common/outcome.hpp"
#include "drand/messages.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"

namespace fc::drand {
  using primitives::ChainEpoch;

  /// Manager for Drand Beacons
  class Beaconizer {
   public:
    virtual ~Beaconizer() = default;

    /// Acquires a beacon from the drand network
    virtual outcome::result<BeaconEntry> entry(uint64_t round) = 0;

    /// Verifies a beacon against the previous
    virtual outcome::result<void> verifyEntry(const BeaconEntry &current,
                                              const BeaconEntry &previous) = 0;

    /// Calculates the maximum beacon round for the given filecoin epoch
    virtual outcome::result<uint64_t> MaxBeaconRoundForEpoch(
        ChainEpoch fil_epoch) = 0;
  };
}  // namespace fc::drand

#endif  // CPP_FILECOIN_CORE_DRAND_BEACONIZER_HPP
