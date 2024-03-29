/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sector_storage/worker.hpp"

#include "api/rpc/client_setup.hpp"
#include "api/worker_api.hpp"

namespace fc::sector_storage {
  using api::CommonApi;
  using api::WorkerApi;
  using api::rpc::Client;
  using boost::asio::io_context;
  using libp2p::multi::Multiaddress;

  class RemoteWorker : public Worker {
   public:
    static outcome::result<std::shared_ptr<RemoteWorker>> connectRemoteWorker(
        io_context &context,
        const std::shared_ptr<CommonApi> &api,
        const std::string &address);

    outcome::result<CallId> addPiece(const SectorRef &sector,
                                     VectorCoW<UnpaddedPieceSize> piece_sizes,
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
        const SectorRef &sector, std::vector<Range> keep_unsealed) override;

    outcome::result<CallId> replicaUpdate(
        const SectorRef &sector, const std::vector<PieceInfo> &pieces) override;

    outcome::result<CallId> proveReplicaUpdate1(
        const SectorRef &sector,
        const CID &sector_key,
        const CID &new_sealed,
        const CID &new_unsealed) override;

    outcome::result<CallId> proveReplicaUpdate2(
        const SectorRef &sector,
        const CID &sector_key,
        const CID &new_sealed,
        const CID &new_unsealed,
        const Update1Output &update_1_output) override;

    outcome::result<CallId> finalizeReplicaUpdate(
        const SectorRef &sector, std::vector<Range> keep_unsealed) override;

    outcome::result<fc::sector_storage::CallId> moveStorage(
        const SectorRef &sector, SectorFileType types) override;

    outcome::result<fc::sector_storage::CallId> unsealPiece(
        const SectorRef &sector,
        UnpaddedByteIndex offset,
        const UnpaddedPieceSize &size,
        const SealRandomness &randomness,
        const CID &unsealed_cid) override;

    outcome::result<fc::sector_storage::CallId> readPiece(
        PieceData output,
        const SectorRef &sector,
        UnpaddedByteIndex offset,
        const UnpaddedPieceSize &size) override;

    outcome::result<fc::sector_storage::CallId> fetch(
        const SectorRef &sector,
        const SectorFileType &file_type,
        PathType path_type,
        AcquireMode mode) override;

    outcome::result<primitives::WorkerInfo> getInfo() override;

    outcome::result<std::set<primitives::TaskType>> getSupportedTask() override;

    outcome::result<std::vector<primitives::StoragePath>> getAccessiblePaths()
        override;

    bool isLocalWorker() const override;

   private:
    explicit RemoteWorker(io_context &context);

    IoThread worker_thread_;
    WorkerApi api_;
    Client wsc_;
    io_context &io_;
    std::string host_;
    std::string port_;
  };
}  // namespace fc::sector_storage
