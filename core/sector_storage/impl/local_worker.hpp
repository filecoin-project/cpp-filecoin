/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sector_storage/worker.hpp"

#include "common/logger.hpp"
#include "proofs/impl/proof_engine_impl.hpp"
#include "sector_storage/stores/impl/local_store.hpp"

namespace fc::sector_storage {

  struct WorkerConfig {
    boost::optional<std::string> custom_hostname = boost::none;
    std::set<primitives::TaskType> task_types;
    bool is_no_swap = false;
  };

  class LocalWorker : public Worker,
                      public std::enable_shared_from_this<LocalWorker> {
   public:
    LocalWorker(std::shared_ptr<boost::asio::io_context> context,
                const WorkerConfig &config,
                std::shared_ptr<WorkerReturn> return_interface,
                std::shared_ptr<stores::RemoteStore> store,
                std::shared_ptr<proofs::ProofEngine> proofs =
                    std::make_shared<proofs::ProofEngineImpl>());

    outcome::result<CallId> addPiece(
        const SectorRef &sector,
        gsl::span<const UnpaddedPieceSize> piece_sizes,
        const UnpaddedPieceSize &new_piece_size,
        PieceData piece_data) override;

    outcome::result<CallId> sealPreCommit1(
        const SectorRef &sector,
        const SealRandomness &ticket,
        const std::vector<PieceInfo> &pieces) override;

    outcome::result<CallId> sealPreCommit2(
        const SectorRef &sector,
        const PreCommit1Output &pre_commit_1_output) override;

    outcome::result<CallId> sealCommit1(const SectorRef &sector,
                                        const SealRandomness &ticket,
                                        const InteractiveRandomness &seed,
                                        const std::vector<PieceInfo> &pieces,
                                        const SectorCids &cids) override;

    outcome::result<CallId> sealCommit2(
        const SectorRef &sector, const Commit1Output &commit_1_output) override;

    outcome::result<CallId> finalizeSector(
        const SectorRef &sector,
        const gsl::span<const Range> &keep_unsealed) override;

    outcome::result<CallId> moveStorage(const SectorRef &sector,
                                        SectorFileType types) override;

    outcome::result<CallId> unsealPiece(const SectorRef &sector,
                                        UnpaddedByteIndex offset,
                                        const UnpaddedPieceSize &size,
                                        const SealRandomness &randomness,
                                        const CID &unsealed_cid) override;

    outcome::result<CallId> readPiece(PieceData output,
                                      const SectorRef &sector,
                                      UnpaddedByteIndex offset,
                                      const UnpaddedPieceSize &size) override;

    outcome::result<CallId> fetch(const SectorRef &sector,
                                  const SectorFileType &file_type,
                                  PathType path_type,
                                  AcquireMode mode) override;

    outcome::result<primitives::WorkerInfo> getInfo() override;

    outcome::result<std::set<primitives::TaskType>> getSupportedTask() override;

    outcome::result<std::vector<primitives::StoragePath>> getAccessiblePaths()
        override;

   private:
    template <typename W, typename R>
    outcome::result<CallId> asyncCall(const SectorRef &sector,
                                      R _return,
                                      W work);

    struct Response {
      stores::SectorPaths paths;
      std::function<void()> release_function;
    };

    outcome::result<Response> acquireSector(
        SectorRef id,
        SectorFileType exisitng,
        SectorFileType allocate,
        PathType path,
        AcquireMode mode = AcquireMode::kCopy);

    std::shared_ptr<boost::asio::io_context> context_;

    std::shared_ptr<stores::RemoteStore> remote_store_;
    std::shared_ptr<stores::SectorIndex> index_;

    std::shared_ptr<proofs::ProofEngine> proofs_;
    std::shared_ptr<WorkerReturn> return_;

    std::set<primitives::TaskType> task_types_;
    bool is_no_swap;
    std::string hostname_;
    common::Logger logger_;
  };
}  // namespace fc::sector_storage
