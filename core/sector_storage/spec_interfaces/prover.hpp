/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "primitives/sector/sector.hpp"

using fc::primitives::sector::ActorId;
using fc::primitives::sector::PoStProof;
using fc::primitives::sector::PoStRandomness;
using fc::primitives::sector::SectorId;
using fc::primitives::sector::SectorInfo;

namespace fc::sector_storage {

  class Prover {
   public:
    struct WindowPoStResponse {
      std::vector<PoStProof> proof;
      std::vector<SectorId> skipped;
    };

    virtual ~Prover() = default;

    virtual outcome::result<std::vector<PoStProof>> generateWinningPoSt(
        ActorId miner_id,
        gsl::span<const SectorInfo> sector_info,
        PoStRandomness randomness) = 0;

    virtual outcome::result<WindowPoStResponse> generateWindowPoSt(
        ActorId miner_id,
        gsl::span<const SectorInfo> sector_info,
        PoStRandomness randomness) = 0;
  };

  inline bool operator==(const Prover::WindowPoStResponse &lhs,
                         const Prover::WindowPoStResponse &rhs) {
    return lhs.proof == rhs.proof && lhs.skipped == rhs.skipped;
  };
}  // namespace fc::sector_storage
