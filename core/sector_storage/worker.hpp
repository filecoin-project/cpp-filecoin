/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTOR_STORAGE_WORKER_HPP
#define CPP_FILECOIN_CORE_SECTOR_STORAGE_WORKER_HPP

#include "common/outcome.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/piece/piece_data.hpp"
#include "primitives/seal_tasks/task.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/sector_file/sector_file.hpp"
#include "proofs/proofs.hpp"
#include "sector_storage/stores/store.hpp"

namespace fc::sector_storage {
  using fc::primitives::piece::PieceInfo;
  using primitives::piece::PieceData;
  using primitives::piece::UnpaddedByteIndex;
  using primitives::piece::UnpaddedPieceSize;
  using primitives::sector::SealRandomness;
  using primitives::sector::SectorId;
  using primitives::sector_file::SectorFileType;
  using PreCommit1Output = proofs::Phase1Output;
  using Commit1Output = proofs::Phase1Output;
  using SectorCids = proofs::SealedAndUnsealedCID;
  using fc::primitives::sector::InteractiveRandomness;
  using fc::primitives::sector::Proof;
  using fc::primitives::sector::SealRandomness;
  using stores::AcquireMode;
  using stores::PathType;

  struct Range {
    UnpaddedPieceSize offset;
    UnpaddedPieceSize size;
  };

  class Worker {
   public:
    virtual ~Worker() = default;

    virtual outcome::result<void> moveStorage(const SectorId &sector) = 0;

    virtual outcome::result<void> fetch(const SectorId &sector,
                                        const SectorFileType &file_type,
                                        PathType path_type,
                                        AcquireMode mode) = 0;

    virtual outcome::result<void> unsealPiece(const SectorId &sector,
                                              UnpaddedByteIndex offset,
                                              const UnpaddedPieceSize &size,
                                              const SealRandomness &randomness,
                                              const CID &unsealed_cid) = 0;

    /**
     * @param output is PieceData with write part of pipe
     */
    virtual outcome::result<bool> readPiece(PieceData output,
                                            const SectorId &sector,
                                            UnpaddedByteIndex offset,
                                            const UnpaddedPieceSize &size) = 0;

    virtual outcome::result<primitives::WorkerInfo> getInfo() = 0;

    virtual outcome::result<std::set<primitives::TaskType>>
    getSupportedTask() = 0;

    virtual outcome::result<std::vector<primitives::StoragePath>>
    getAccessiblePaths() = 0;

    virtual outcome::result<PreCommit1Output> sealPreCommit1(
        const SectorId &sector,
        const SealRandomness &ticket,
        gsl::span<const PieceInfo> pieces) = 0;

    virtual outcome::result<SectorCids> sealPreCommit2(
        const SectorId &sector,
        const PreCommit1Output &pre_commit_1_output) = 0;

    virtual outcome::result<Commit1Output> sealCommit1(
        const SectorId &sector,
        const SealRandomness &ticket,
        const InteractiveRandomness &seed,
        gsl::span<const PieceInfo> pieces,
        const SectorCids &cids) = 0;

    virtual outcome::result<Proof> sealCommit2(
        const SectorId &sector, const Commit1Output &commit_1_output) = 0;

    virtual outcome::result<void> finalizeSector(
        const SectorId &sector,
        const gsl::span<const Range> &keep_unsealed) = 0;

    virtual outcome::result<void> remove(const SectorId &sector) = 0;

    virtual outcome::result<PieceInfo> addPiece(
        const SectorId &sector,
        gsl::span<const UnpaddedPieceSize> piece_sizes,
        const UnpaddedPieceSize &new_piece_size,
        const proofs::PieceData &piece_data) = 0;
  };

  enum class WorkerErrors {
    kCannotCreateSealedFile = 1,
    kCannotCreateCacheDir,
    kCannotRemoveCacheDir,
    kPiecesDoNotMatchSectorSize,
    kCannotCreateTempFile,
    kCannotGetNumberOfCPUs,
    kCannotGetVMStat,
    kCannotGetPageSize,
    kCannotOpenMemInfoFile,
    kCannotRemoveSector,
    kUnsupportedPlatform,
    kOutOfBound,
    kCannotOpenFile,
  };
}  // namespace fc::sector_storage

OUTCOME_HPP_DECLARE_ERROR(fc::sector_storage, WorkerErrors);

#endif  // CPP_FILECOIN_CORE_SECTOR_STORAGE_WORKER_HPP
