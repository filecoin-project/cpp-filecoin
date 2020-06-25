/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SECTOR_STROAGE_SPEC_INTERFACES_SEALER_HPP
#define CPP_FILECOIN_SECTOR_STROAGE_SPEC_INTERFACES_SEALER_HPP

#include "common/outcome.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/sector_file/sector_file.hpp"
#include "proofs/proofs.hpp"

namespace fc::sector_storage {
  using fc::primitives::piece::PieceInfo;
  using fc::primitives::piece::UnpaddedPieceSize;
  using fc::primitives::sector::SectorId;
  using PreCommit1Output = fc::proofs::Phase1Output;
  using Commit1Output = fc::proofs::Phase1Output;
  using SectorCids = fc::proofs::SealedAndUnsealedCID;
  using fc::primitives::sector::InteractiveRandomness;
  using fc::primitives::sector::Proof;
  using fc::primitives::sector::SealRandomness;

  struct Range {
    UnpaddedPieceSize offset;
    UnpaddedPieceSize size;
  };

  class Sealer {
   public:
    virtual ~Sealer() = default;

    virtual outcome::result<PreCommit1Output> sealPreCommit1(
        const SectorId &sector,
        const SealRandomness &ticket,
        gsl::span<const PieceInfo> pieces) = 0;

    virtual outcome::result<SectorCids> sealPreCommit2(
        const SectorId &sector, const PreCommit1Output &pc1o) = 0;

    virtual outcome::result<Commit1Output> sealCommit1(
        const SectorId &sector,
        const SealRandomness &ticket,
        const InteractiveRandomness &seed,
        gsl::span<const PieceInfo> pieces,
        const SectorCids &cids) = 0;

    virtual outcome::result<Proof> sealCommit2(const SectorId &sector,
                                               const Commit1Output &c1o) = 0;

    virtual outcome::result<void> finalizeSector(
        const SectorId &sector, const gsl::span<Range> &keep_unsealed) = 0;

    // ReleaseUnsealed marks parts of the unsealed sector file as safe to drop
    // (called by the fsm on restart, allows storage to keep no persistent state
    // about unsealed fast-retrieval copies)
    virtual outcome::result<void> releaseUnsealed(
        const SectorId &sector, const gsl::span<Range> &safe_to_free) = 0;

    virtual outcome::result<void> remove(const SectorId &sector) = 0;
  };
}  // namespace fc::sector_storage

#endif  // CPP_FILECOIN_SECTOR_STROAGE_SPEC_INTERFACES_SEALER_HPP
