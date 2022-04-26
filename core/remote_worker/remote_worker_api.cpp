/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "remote_worker/remote_worker_api.hpp"

namespace fc::remote_worker {
  using api::VersionResult;
  using primitives::piece::PieceInfo;
  using primitives::piece::UnpaddedByteIndex;
  using primitives::piece::UnpaddedPieceSize;
  using primitives::sector::SealRandomness;
  using primitives::sector::SectorRef;
  using sector_storage::AcquireMode;
  using sector_storage::Commit1Output;
  using sector_storage::InteractiveRandomness;
  using sector_storage::PathType;
  using sector_storage::PreCommit1Output;
  using sector_storage::Range;
  using sector_storage::SectorCids;
  using sector_storage::SectorFileType;
  using sector_storage::Update1Output;

  std::shared_ptr<WorkerApi> makeWorkerApi(
      const std::shared_ptr<LocalStore> &local_store,
      const std::shared_ptr<LocalWorker> &worker) {
    auto worker_api{std::make_shared<api::WorkerApi>()};
    worker_api->Version = []() { return VersionResult{"seal-worker", 0, 0}; };
    worker_api->StorageAddLocal = [=](const std::string &path) {
      return local_store->openPath(path);
    };
    worker_api->Fetch = [=](const SectorRef &sector,
                            const SectorFileType &file_type,
                            PathType path_type,
                            AcquireMode mode) {
      return worker->fetch(sector, file_type, path_type, mode);
    };
    worker_api->UnsealPiece = [=](const SectorRef &sector,
                                  UnpaddedByteIndex offset,
                                  const UnpaddedPieceSize &size,
                                  const SealRandomness &randomness,
                                  const CID &unsealed_cid) {
      return worker->unsealPiece(
          sector, offset, size, randomness, unsealed_cid);
    };
    worker_api->MoveStorage = [=](const SectorRef &sector,
                                  const SectorFileType &types) {
      return worker->moveStorage(sector, types);
    };

    worker_api->Info = [=]() { return worker->getInfo(); };
    worker_api->Paths = [=]() { return worker->getAccessiblePaths(); };
    worker_api->TaskTypes =
        [=]() -> outcome::result<api::CodecSetAsMap<primitives::TaskType>> {
      OUTCOME_TRY(supported_tasks, worker->getSupportedTask());
      // N.B. set encoded as JSON map
      return api::CodecSetAsMap<primitives::TaskType>{
          std::move(supported_tasks)};
    };

    worker_api->SealPreCommit1 = [=](const SectorRef &sector,
                                     const SealRandomness &ticket,
                                     const std::vector<PieceInfo> &pieces) {
      return worker->sealPreCommit1(sector, ticket, pieces);
    };
    worker_api->SealPreCommit2 =
        [=](const SectorRef &sector,
            const PreCommit1Output &pre_commit_1_output) {
          return worker->sealPreCommit2(sector, pre_commit_1_output);
        };
    worker_api->SealCommit1 = [=](const SectorRef &sector,
                                  const SealRandomness &ticket,
                                  const InteractiveRandomness &seed,
                                  const std::vector<PieceInfo> &pieces,
                                  const SectorCids &cids) {
      return worker->sealCommit1(sector, ticket, seed, pieces, cids);
    };
    worker_api->SealCommit2 = [=](const SectorRef &sector,
                                  const Commit1Output &commit_1_output) {
      return worker->sealCommit2(sector, commit_1_output);
    };
    worker_api->FinalizeSector = [=](const SectorRef &sector,
                                     std::vector<Range> keep_unsealed) {
      return worker->finalizeSector(sector, std::move(keep_unsealed));
    };

    worker_api->ReplicaUpdate = [=](const SectorRef &sector,
                                    const std::vector<PieceInfo> &pieces) {
      return worker->replicaUpdate(sector, pieces);
    };
    worker_api->ProveReplicaUpdate1 = [=](const SectorRef &sector,
                                          const CID &sector_key,
                                          const CID &new_sealed,
                                          const CID &new_unsealed) {
      return worker->proveReplicaUpdate1(
          sector, sector_key, new_sealed, new_unsealed);
    };
    worker_api->ProveReplicaUpdate2 = [=](const SectorRef &sector,
                                          const CID &sector_key,
                                          const CID &new_sealed,
                                          const CID &new_unsealed,
                                          const Update1Output &update1_output) {
      return worker->proveReplicaUpdate2(
          sector, sector_key, new_sealed, new_unsealed, update1_output);
    };

    return worker_api;
  };

}  // namespace fc::remote_worker
