/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTOR_STORAGE_IMPL_LOCAL_WORKER_HPP
#define CPP_FILECOIN_CORE_SECTOR_STORAGE_IMPL_LOCAL_WORKER_HPP

#include "sector_storage/worker.hpp"

#include "common/logger.hpp"
#include "sector_storage/stores/impl/local_store.hpp"

namespace fc::sector_storage {

  struct WorkerConfig {
    std::string hostname;
    primitives::sector::RegisteredProof seal_proof_type;
    std::set<primitives::TaskType> task_types;
  };

  class LocalWorker : public Worker {
   public:
    LocalWorker(WorkerConfig config,
                std::shared_ptr<stores::RemoteStore> store);

    outcome::result<PreCommit1Output> sealPreCommit1(
        const SectorId &sector,
        const SealRandomness &ticket,
        gsl::span<const PieceInfo> pieces) override;

    outcome::result<SectorCids> sealPreCommit2(
        const SectorId &sector,
        const PreCommit1Output &pre_commit_1_output) override;

    outcome::result<Commit1Output> sealCommit1(
        const SectorId &sector,
        const SealRandomness &ticket,
        const InteractiveRandomness &seed,
        gsl::span<const PieceInfo> pieces,
        const SectorCids &cids) override;

    outcome::result<Proof> sealCommit2(
        const SectorId &sector, const Commit1Output &commit_1_output) override;

    outcome::result<void> finalizeSector(const SectorId &sector) override;

    outcome::result<void> moveStorage(const SectorId &sector) override;

    outcome::result<void> fetch(const SectorId &sector,
                                const SectorFileType &file_type,
                                bool can_seal) override;

    outcome::result<void> unsealPiece(const SectorId &sector,
                                      UnpaddedByteIndex offset,
                                      const UnpaddedPieceSize &size,
                                      const SealRandomness &randomness,
                                      const CID &unsealed_cid) override;

    outcome::result<void> readPiece(proofs::PieceData output,
                                    const SectorId &sector,
                                    UnpaddedByteIndex offset,
                                    const UnpaddedPieceSize &size) override;

    outcome::result<primitives::WorkerInfo> getInfo() override;

    outcome::result<std::set<primitives::TaskType>> getSupportedTask() override;

    outcome::result<std::vector<primitives::StoragePath>> getAccessiblePaths()
        override;

    outcome::result<void> remove(const SectorId &sector) override;

    outcome::result<PieceInfo> addPiece(
        const SectorId &sector,
        gsl::span<const UnpaddedPieceSize> piece_sizes,
        const UnpaddedPieceSize &new_piece_size,
        const proofs::PieceData &piece_data) override;

   private:
    std::shared_ptr<stores::RemoteStore> remote_store_;
    std::shared_ptr<stores::LocalStore> local_store_;
    std::shared_ptr<stores::SectorIndex> index_;

    WorkerConfig config_;
    common::Logger logger_;
  };

}  // namespace fc::sector_storage

#endif  // CPP_FILECOIN_CORE_SECTOR_STORAGE_IMPL_LOCAL_WORKER_HPP
