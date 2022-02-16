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
  using ReplicaUpdateProof = Bytes;
  using ReplicaVanillaProofs = std::vector<Bytes>;

  class Manager : public Prover, public FaultTracker {
   public:
    virtual outcome::result<void> addLocalStorage(const std::string &path) = 0;

    virtual outcome::result<void> addWorker(std::shared_ptr<Worker> worker) = 0;

    virtual outcome::result<std::unordered_map<StorageID, std::string>>
    getLocalStorages() = 0;

    virtual outcome::result<FsStat> getFsStat(StorageID storage_id) = 0;

    virtual std::shared_ptr<proofs::ProofEngine> getProofEngine() const = 0;

    virtual void readPiece(PieceData output,
                           const SectorRef &sector,
                           UnpaddedByteIndex offset,
                           const UnpaddedPieceSize &size,
                           const SealRandomness &randomness,
                           const CID &cid,
                           const std::function<void(outcome::result<bool>)> &cb,
                           uint64_t priority) = 0;

    // Sealer
    virtual void sealPreCommit1(
        const SectorRef &sector,
        const SealRandomness &ticket,
        const std::vector<PieceInfo> &pieces,
        const std::function<void(outcome::result<PreCommit1Output>)> &cb,
        uint64_t priority) = 0;

    virtual void sealPreCommit2(
        const SectorRef &sector,
        const PreCommit1Output &pre_commit_1_output,
        const std::function<void(outcome::result<SectorCids>)> &cb,
        uint64_t priority) = 0;

    virtual void sealCommit1(
        const SectorRef &sector,
        const SealRandomness &ticket,
        const InteractiveRandomness &seed,
        const std::vector<PieceInfo> &pieces,
        const SectorCids &cids,
        const std::function<void(outcome::result<Commit1Output>)> &cb,
        uint64_t priority) = 0;

    virtual void sealCommit2(
        const SectorRef &sector,
        const Commit1Output &commit_1_output,
        const std::function<void(outcome::result<Proof>)> &cb,
        uint64_t priority) = 0;

    virtual void finalizeSector(
        const SectorRef &sector,
        std::vector<Range> keep_unsealed,
        const std::function<void(outcome::result<void>)> &cb,
        uint64_t priority) = 0;

    virtual outcome::result<void> remove(const SectorRef &sector) = 0;

    /** Generate Snap Deals replica update */
    virtual void replicaUpdate(
        const SectorRef &sector,
        const std::vector<PieceInfo> &pieces,
        const std::function<void(outcome::result<ReplicaUpdateOut>)> &cb,
        uint64_t priority) = 0;

    /** Prove Snap Deals replica update */
    virtual void proveReplicaUpdate1(
        const SectorRef &sector,
        const CID &sector_key,
        const CID &new_sealed,
        const CID &new_unsealed,
        const std::function<void(outcome::result<ReplicaVanillaProofs>)> &cb,
        uint64_t priority) = 0;

    virtual void proveReplicaUpdate2(
        const SectorRef &sector,
        const CID &sector_key,
        const CID &new_sealed,
        const CID &new_unsealed,
        const Update1Output &update_1_output,
        const std::function<void(outcome::result<ReplicaUpdateProof>)> &cb,
        uint64_t priority) = 0;

    // Storage
    virtual void addPiece(
        const SectorRef &sector,
        VectorCoW<UnpaddedPieceSize> piece_sizes,
        const UnpaddedPieceSize &new_piece_size,
        proofs::PieceData piece_data,
        const std::function<void(outcome::result<PieceInfo>)> &cb,
        uint64_t priority) = 0;

    virtual outcome::result<PieceInfo> addPieceSync(
        const SectorRef &sector,
        VectorCoW<UnpaddedPieceSize> piece_sizes,
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
