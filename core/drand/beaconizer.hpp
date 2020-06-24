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
    virtual outcome::result<uint64_t> maxBeaconRoundForEpoch(
        ChainEpoch fil_epoch) = 0;

    inline outcome::result<std::vector<BeaconEntry>> beaconEntriesForBlock(
        ChainEpoch fil_epoch, const BeaconEntry &prev) {
      std::vector<BeaconEntry> beacons;
      OUTCOME_TRY(max_round, maxBeaconRoundForEpoch(fil_epoch));
      if (max_round != prev.round) {
        auto prev_round{prev.round == 0 ? max_round - 1 : prev.round};
        for (auto round{max_round}; round > prev_round; --round) {
          OUTCOME_TRY(beacon, entry(round));
          BOOST_ASSERT(beacon.round == round);
          beacons.push_back(std::move(beacon));
        }
        std::reverse(beacons.begin(), beacons.end());
      }
      return beacons;
    }
  };
}  // namespace fc::drand

#endif  // CPP_FILECOIN_CORE_DRAND_BEACONIZER_HPP
