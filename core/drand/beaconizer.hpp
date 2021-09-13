/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/async.hpp"
#include "drand/messages.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"

namespace fc::drand {
  using primitives::ChainEpoch;

  /// Manager for Drand Beacons
  class Beaconizer {
   public:
    virtual ~Beaconizer() = default;

    /// Acquires a beacon from the drand network
    virtual void entry(Round round, CbT<BeaconEntry> cb) = 0;

    /// Verifies a beacon against the previous
    virtual outcome::result<void> verifyEntry(const BeaconEntry &current,
                                              const BeaconEntry &previous) = 0;
  };

  struct DrandSchedule {
    virtual ~DrandSchedule() = default;

    /// Calculates the maximum beacon round for the given filecoin epoch
    virtual Round maxRound(ChainEpoch epoch) const = 0;
  };
}  // namespace fc::drand
