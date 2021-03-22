/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/policy.hpp"
#include "const.hpp"

namespace fc::vm::actor::builtin::types::miner {
  ChainEpoch kWPoStProvingPeriod = kEpochsInDay;
  EpochDuration kWPoStChallengeWindow = 30 * 60 / kEpochDurationSeconds;
  EpochDuration kFaultMaxAge{kWPoStProvingPeriod * 14};
  ChainEpoch kMinSectorExpiration = 180 * kEpochsInDay;
  std::set<RegisteredSealProof> kSupportedProofs{
      RegisteredSealProof::kStackedDrg32GiBV1,
      RegisteredSealProof::kStackedDrg64GiBV1,
  };
  ChainEpoch kMaxSectorExpirationExtension = 540 * kEpochsInDay;
  EpochDuration kMaxProveCommitDuration =
      kEpochsInDay + kPreCommitChallengeDelay;

  void setPolicy(size_t epochDurationSeconds,
                 std::set<RegisteredSealProof> supportedProofs) {
    kWPoStChallengeWindow = 30 * 60 / kEpochDurationSeconds;

    const auto epochsInHour = kSecondsInHour / epochDurationSeconds;
    const auto epochsInDay = 24 * epochsInHour;
    kWPoStProvingPeriod = epochsInDay;

    kFaultMaxAge = kWPoStProvingPeriod * 14;
    kMinSectorExpiration = 180 * epochsInDay;
    kMaxSectorExpirationExtension = 540 * epochsInDay;
    kMaxProveCommitDuration = epochsInDay + kPreCommitChallengeDelay;

    kSupportedProofs = std::move(supportedProofs);
  }

}  // namespace fc::vm::actor::builtin::types::miner
