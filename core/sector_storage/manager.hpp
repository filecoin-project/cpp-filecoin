/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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
    virtual outcome::result<void> addLocalStorage(const std::string &path) = 0;

    virtual outcome::result<void> addWorker(std::shared_ptr<Worker> worker) = 0;

    virtual outcome::result<std::unordered_map<StorageID, std::string>>
    getLocalStorages() = 0;

    virtual outcome::result<FsStat> getFsStat(StorageID storage_id) = 0;

    virtual std::shared_ptr<proofs::ProofEngine> getProofEngine() const = 0;

    virtual outcome::result<void> readPiece(
        PieceData output,
        const SectorRef &sector,
        UnpaddedByteIndex offset,
        const UnpaddedPieceSize &size,
        const SealRandomness &randomness,
        const CID &cid,
        std::function<void(outcome::result<bool>)> cb) = 0;

    virtual outcome::result<bool> readPieceSync(
        PieceData output,
        const SectorRef &sector,
        UnpaddedByteIndex offset,
        const UnpaddedPieceSize &size,
        const SealRandomness &randomness,
        const CID &cid) = 0;

    // Sealer
    virtual outcome::result<void> sealPreCommit1(
        const SectorRef &sector,
        const SealRandomness &ticket,
        gsl::span<const PieceInfo> pieces,
        std::function<void(outcome::result<PreCommit1Output>)> cb,
        uint64_t priority) = 0;

    virtual outcome::result<PreCommit1Output> sealPreCommit1Sync(
        const SectorRef &sector,
        const SealRandomness &ticket,
        gsl::span<const PieceInfo> pieces,
        uint64_t priority) = 0;

    virtual outcome::result<void> sealPreCommit2(
        const SectorRef &sector,
        const PreCommit1Output &pre_commit_1_output,
        std::function<void(outcome::result<SectorCids>)> cb,
        uint64_t priority) = 0;

    virtual outcome::result<SectorCids> sealPreCommit2Sync(
        const SectorRef &sector,
        const PreCommit1Output &pre_commit_1_output,
        uint64_t priority) = 0;

    virtual outcome::result<void> sealCommit1(
        const SectorRef &sector,
        const SealRandomness &ticket,
        const InteractiveRandomness &seed,
        gsl::span<const PieceInfo> pieces,
        const SectorCids &cids,
        std::function<void(outcome::result<Commit1Output>)> cb,
        uint64_t priority) = 0;

    virtual outcome::result<Commit1Output> sealCommit1Sync(
        const SectorRef &sector,
        const SealRandomness &ticket,
        const InteractiveRandomness &seed,
        gsl::span<const PieceInfo> pieces,
        const SectorCids &cids,
        uint64_t priority) = 0;

    virtual outcome::result<void> sealCommit2(
        const SectorRef &sector,
        const Commit1Output &commit_1_output,
        std::function<void(outcome::result<Proof>)> cb,
        uint64_t priority) = 0;

    virtual outcome::result<Proof> sealCommit2Sync(
        const SectorRef &sector,
        const Commit1Output &commit_1_output,
        uint64_t priority) = 0;

    virtual outcome::result<void> finalizeSector(
        const SectorRef &sector,
        const gsl::span<const Range> &keep_unsealed,
        std::function<void(outcome::result<void>)> cb,
        uint64_t priority) = 0;

    virtual outcome::result<void> finalizeSectorSync(
        const SectorRef &sector,
        const gsl::span<const Range> &keep_unsealed,
        uint64_t priority) = 0;

    virtual outcome::result<void> remove(const SectorRef &sector) = 0;

    // Storage
    virtual outcome::result<void> addPiece(
        const SectorRef &sector,
        gsl::span<const UnpaddedPieceSize> piece_sizes,
        const UnpaddedPieceSize &new_piece_size,
        proofs::PieceData piece_data,
        std::function<void(outcome::result<PieceInfo>)> cb,
        uint64_t priority) = 0;

    virtual outcome::result<PieceInfo> addPieceSync(
        const SectorRef &sector,
        gsl::span<const UnpaddedPieceSize> piece_sizes,
        const UnpaddedPieceSize &new_piece_size,
        proofs::PieceData piece_data,
        uint64_t priority) = 0;
  };

  enum class ManagerErrors {
    kCannotGetHomeDir = 1,
    kSomeSectorSkipped,
    kCannotLock,
    kReadOnly,
    kCannotReadData,
  };
}  // namespace fc::sector_storage

OUTCOME_HPP_DECLARE_ERROR(fc::sector_storage, ManagerErrors);
