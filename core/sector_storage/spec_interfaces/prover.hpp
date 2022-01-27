/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "primitives/sector/sector.hpp"

namespace fc::sector_storage {
  using primitives::sector::ActorId;
  using primitives::sector::PoStProof;
  using primitives::sector::PoStRandomness;
  using primitives::sector::SectorId;
  using primitives::sector::ExtendedSectorInfo;

  class Prover {
   public:
    struct WindowPoStResponse {
      std::vector<PoStProof> proof;
      std::vector<SectorId> skipped;
    };

    virtual ~Prover() = default;

    virtual outcome::result<std::vector<PoStProof>> generateWinningPoSt(
        ActorId miner_id,
        gsl::span<const ExtendedSectorInfo> sector_info,
        PoStRandomness randomness) = 0;

    virtual outcome::result<WindowPoStResponse> generateWindowPoSt(
        ActorId miner_id,
        gsl::span<const ExtendedSectorInfo> sector_info,
        PoStRandomness randomness) = 0;
  };

  inline bool operator==(const Prover::WindowPoStResponse &lhs,
                         const Prover::WindowPoStResponse &rhs) {
    return lhs.proof == rhs.proof && lhs.skipped == rhs.skipped;
  };
}  // namespace fc::sector_storage
