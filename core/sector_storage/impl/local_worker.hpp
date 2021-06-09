/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/common/metrics/instance_count.hpp>

#include "sector_storage/worker.hpp"

#include "common/logger.hpp"
#include "proofs/impl/proof_engine_impl.hpp"
#include "sector_storage/stores/impl/local_store.hpp"

namespace fc::sector_storage {

  struct WorkerConfig {
    primitives::sector::RegisteredSealProof seal_proof_type;
    std::set<primitives::TaskType> task_types;
  };

  class LocalWorker : public Worker {
   public:
    LocalWorker(WorkerConfig config,
                std::shared_ptr<stores::RemoteStore> store,
                std::shared_ptr<proofs::ProofEngine> proofs =
                    std::make_shared<proofs::ProofEngineImpl>());

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

    outcome::result<void> finalizeSector(
        const SectorId &sector,
        const gsl::span<const Range> &keep_unsealed) override;

    outcome::result<void> moveStorage(const SectorId &sector) override;

    outcome::result<void> fetch(const SectorId &sector,
                                const SectorFileType &file_type,
                                PathType path_type,
                                AcquireMode mode) override;

    outcome::result<void> unsealPiece(const SectorId &sector,
                                      UnpaddedByteIndex offset,
                                      const UnpaddedPieceSize &size,
                                      const SealRandomness &randomness,
                                      const CID &unsealed_cid) override;

    outcome::result<bool> readPiece(proofs::PieceData output,
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
    struct Response {
      stores::SectorPaths paths;
      std::function<void()> release_function;
    };

    outcome::result<Response> acquireSector(
        SectorId sector_id,
        SectorFileType exisitng,
        SectorFileType allocate,
        PathType path,
        AcquireMode mode = AcquireMode::kCopy);

    std::shared_ptr<stores::RemoteStore> remote_store_;
    std::shared_ptr<stores::SectorIndex> index_;

    std::shared_ptr<proofs::ProofEngine> proofs_;

    WorkerConfig config_;
    std::string hostname_;
    common::Logger logger_;

    LIBP2P_METRICS_INSTANCE_COUNT(fc::sector_storage::LocalWorker);
  };

}  // namespace fc::sector_storage
