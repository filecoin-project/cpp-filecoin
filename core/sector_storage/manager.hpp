/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTOR_STORAGE_MANAGER_HPP
#define CPP_FILECOIN_CORE_SECTOR_STORAGE_MANAGER_HPP

#include "common/outcome.hpp"
#include "sector_storage/fault_tracker.hpp"
#include "sector_storage/spec_interfaces/prover.hpp"
#include "sector_storage/worker.hpp"

namespace fc::sector_storage {
  using primitives::FsStat;
  using primitives::SectorSize;
  using primitives::StorageID;
  using primitives::piece::PieceData;
  using primitives::piece::UnpaddedByteIndex;

  class Manager : public Prover, public FaultTracker {
   public:
    virtual SectorSize getSectorSize() = 0;

    virtual outcome::result<void> ReadPiece(PieceData output,
                                            const SectorId &sector,
                                            UnpaddedByteIndex offset,
                                            const UnpaddedPieceSize &size,
                                            const SealRandomness &randomness,
                                            const CID &cid) = 0;

    virtual outcome::result<void> addLocalStorage(const std::string &path) = 0;

    virtual outcome::result<void> addWorker(std::shared_ptr<Worker> worker) = 0;

    virtual outcome::result<std::unordered_map<StorageID, std::string>>
    getLocalStorages() = 0;

    virtual outcome::result<FsStat> getFsStat(StorageID storage_id) = 0;

    // Sealer
    virtual outcome::result<PreCommit1Output> sealPreCommit1(
        const SectorId &sector,
        const SealRandomness &ticket,
        gsl::span<const PieceInfo> pieces,
        uint64_t priority) = 0;

    virtual outcome::result<SectorCids> sealPreCommit2(
        const SectorId &sector,
        const PreCommit1Output &pre_commit_1_output,
        uint64_t priority) = 0;

    virtual outcome::result<Commit1Output> sealCommit1(
        const SectorId &sector,
        const SealRandomness &ticket,
        const InteractiveRandomness &seed,
        gsl::span<const PieceInfo> pieces,
        const SectorCids &cids,
        uint64_t priority) = 0;

    virtual outcome::result<Proof> sealCommit2(
        const SectorId &sector,
        const Commit1Output &commit_1_output,
        uint64_t priority) = 0;

    virtual outcome::result<void> finalizeSector(const SectorId &sector,
                                                 uint64_t priority) = 0;

    virtual outcome::result<void> remove(const SectorId &sector) = 0;

    // Storage
    virtual outcome::result<PieceInfo> addPiece(
        const SectorId &sector,
        gsl::span<const UnpaddedPieceSize> piece_sizes,
        const UnpaddedPieceSize &new_piece_size,
        const proofs::PieceData &piece_data,
        uint64_t priority) = 0;
  };

  enum class ManagerErrors {
    kCannotGetHomeDir = 1,
    kSomeSectorSkipped,
  };
}  // namespace fc::sector_storage

OUTCOME_HPP_DECLARE_ERROR(fc::sector_storage, ManagerErrors);

#endif  // CPP_FILECOIN_CORE_SECTOR_STORAGE_MANAGER_HPP
