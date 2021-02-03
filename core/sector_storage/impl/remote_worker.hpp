/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sector_storage/worker.hpp"

#include "api/rpc/wsc.hpp"
#include "api/worker_api.hpp"

namespace fc::sector_storage {
  using api::WorkerApi;
  using api::Api;
  using api::rpc::Client;
  using boost::asio::io_context;
    using libp2p::multi::Multiaddress;

  class RemoteWorker : Worker {
   public:
    static outcome::result<std::shared_ptr<RemoteWorker>> connectRemoteWorker(
        io_context &context, const std::shared_ptr<Api>& full_api, const Multiaddress &address);

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

    outcome::result<bool> readPiece(PieceData output,
                                    const SectorId &sector,
                                    UnpaddedByteIndex offset,
                                    const UnpaddedPieceSize &size) override;

    outcome::result<primitives::WorkerInfo> getInfo() override;

    outcome::result<std::set<primitives::TaskType>> getSupportedTask() override;

    outcome::result<std::vector<primitives::StoragePath>> getAccessiblePaths()
        override;

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

    outcome::result<void> remove(const SectorId &sector) override;

    outcome::result<PieceInfo> addPiece(
        const SectorId &sector,
        gsl::span<const UnpaddedPieceSize> piece_sizes,
        const UnpaddedPieceSize &new_piece_size,
        const proofs::PieceData &piece_data) override;

   private:
    RemoteWorker(io_context &context);

    WorkerApi api_;
    Client wsc_;
  };
}  // namespace fc::sector_storage
