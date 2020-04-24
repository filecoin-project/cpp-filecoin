/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTOR_STORAGE_HPP
#define CPP_FILECOIN_CORE_SECTOR_STORAGE_HPP

#include "primitives/piece/piece.hpp"
#include "primitives/piece/piece_data.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/sector_file/sector_file.hpp"
#include "proofs/proofs.hpp"

namespace fc::sector_storage {
  using fc::primitives::sector_file::SectorPaths;

  using fc::primitives::piece::PieceData;
  using fc::primitives::piece::PieceInfo;
  using fc::primitives::piece::UnpaddedPieceSize;
  using fc::primitives::sector::SectorId;
  using fc::primitives::sector_file::SectorFileType;
  using PreCommit1Output = fc::proofs::Phase1Output;
  using Commit1Output = fc::proofs::Phase1Output;
  using SectorCids = fc::proofs::SealedAndUnsealedCID;
  using fc::primitives::sector::InteractiveRandomness;
  using fc::primitives::sector::Proof;
  using fc::primitives::sector::SealRandomness;

  class SectorStorage {
   public:
    virtual ~SectorStorage() = default;

    // SECTOR PROVIDER
    virtual outcome::result<SectorPaths> acquireSector(
        SectorId id, SectorFileType sector_type) = 0;

    // SEALER
    virtual outcome::result<PreCommit1Output> sealPreCommit1(
        const SectorId &sector,
        const SealRandomness &ticket,
        gsl::span<const PieceInfo> pieces) = 0;

    virtual outcome::result<SectorCids> sealPreCommit2(
        const SectorId &sector, const PreCommit1Output &pc10) = 0;

    virtual outcome::result<Commit1Output> sealCommit1(
        const SectorId &sector,
        const SealRandomness &ticket,
        const InteractiveRandomness &seed,
        gsl::span<const PieceInfo> pieces,
        const SectorCids &cids) = 0;

    virtual outcome::result<Proof> sealCommit2(const SectorId &sector,
                                               const Commit1Output &c1o) = 0;

    virtual outcome::result<void> finalizeSector(const SectorId &sector) = 0;

    // STORAGE
    virtual outcome::result<PieceInfo> addPiece(
        SectorId sector,
        gsl::span<const UnpaddedPieceSize> piece_sizes,
        UnpaddedPieceSize new_piece_size,
        const PieceData &piece_data) = 0;
  };
}  // namespace fc::sector_storage

#endif  // CPP_FILECOIN_CORE_SECTOR_STORAGE_HPP
