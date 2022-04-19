/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/utils.hpp"
#include "api/version.hpp"
#include "codec/json/basic_coding.hpp"
#include "common/outcome.hpp"
#include "primitives/jwt/jwt.hpp"
#include "sector_storage/worker.hpp"

namespace fc::api {
  using codec::json::CodecSetAsMap;
  using primitives::jwt::kAdminPermission;
  using primitives::piece::MetaPieceData;
  using primitives::piece::PieceInfo;
  using primitives::piece::UnpaddedByteIndex;
  using primitives::piece::UnpaddedPieceSize;
  using primitives::sector::InteractiveRandomness;
  using primitives::sector::SealRandomness;
  using primitives::sector::SectorRef;
  using sector_storage::AcquireMode;
  using sector_storage::CallId;
  using sector_storage::Commit1Output;
  using sector_storage::PathType;
  using sector_storage::PreCommit1Output;
  using sector_storage::Proof;
  using sector_storage::Range;
  using sector_storage::SectorCids;
  using sector_storage::SectorFileType;
  using sector_storage::Update1Output;

  struct WorkerApi {
    API_METHOD(AddPiece,
               kAdminPermission,
               CallId,
               SectorRef,
               std::vector<UnpaddedPieceSize>,
               UnpaddedPieceSize,
               MetaPieceData)

    API_METHOD(Fetch,
               kAdminPermission,
               CallId,
               const SectorRef &,
               const SectorFileType &,
               PathType,
               AcquireMode)

    API_METHOD(FinalizeSector,
               kAdminPermission,
               CallId,
               const SectorRef &,
               const std::vector<Range> &)

    API_METHOD(ReplicaUpdate,
               kAdminPermission,
               CallId,
               const SectorRef &,
               const std::vector<PieceInfo> &)
    API_METHOD(ProveReplicaUpdate1,
               kAdminPermission,
               CallId,
               const SectorRef &,
               const CID &,
               const CID &,
               const CID &)
    API_METHOD(ProveReplicaUpdate2,
               kAdminPermission,
               CallId,
               const SectorRef &,
               const CID &,
               const CID &,
               const CID &,
               const Update1Output &)
    API_METHOD(FinalizeReplicaUpdate,
               kAdminPermission,
               CallId,
               const SectorRef &,
               const std::vector<Range> &)

    API_METHOD(Info, kAdminPermission, primitives::WorkerInfo)

    API_METHOD(MoveStorage,
               kAdminPermission,
               CallId,
               const SectorRef &,
               SectorFileType)

    API_METHOD(Paths, kAdminPermission, std::vector<primitives::StoragePath>)

    API_METHOD(SealCommit1,
               kAdminPermission,
               CallId,
               const SectorRef &,
               const SealRandomness &,
               const InteractiveRandomness &,
               std::vector<PieceInfo>,
               const SectorCids &)

    API_METHOD(SealCommit2,
               kAdminPermission,
               CallId,
               const SectorRef &,
               const Commit1Output &)

    API_METHOD(SealPreCommit1,
               kAdminPermission,
               CallId,
               const SectorRef &,
               const SealRandomness &,
               std::vector<PieceInfo>)

    API_METHOD(SealPreCommit2,
               kAdminPermission,
               CallId,
               const SectorRef &,
               const PreCommit1Output &)

    API_METHOD(StorageAddLocal, kAdminPermission, void, const std::string &)

    API_METHOD(TaskTypes, kAdminPermission, CodecSetAsMap<primitives::TaskType>)

    API_METHOD(UnsealPiece,
               kAdminPermission,
               CallId,
               const SectorRef &,
               UnpaddedByteIndex,
               const UnpaddedPieceSize &,
               const SealRandomness &,
               const CID &)

    API_METHOD(Version, kAdminPermission, VersionResult)
  };

  template <typename A, typename F>
  void visit(const WorkerApi &, A &&a, const F &f) {
    f(a.AddPiece);
    f(a.Fetch);
    f(a.FinalizeSector);
    f(a.ReplicaUpdate);
    f(a.ProveReplicaUpdate1);
    f(a.ProveReplicaUpdate2);
    f(a.FinalizeReplicaUpdate);
    f(a.Info);
    f(a.MoveStorage);
    f(a.Paths);
    f(a.SealCommit1);
    f(a.SealCommit2);
    f(a.SealPreCommit1);
    f(a.SealPreCommit2);
    f(a.StorageAddLocal);
    f(a.TaskTypes);
    f(a.UnsealPiece);
    f(a.Version);
  }
}  // namespace fc::api
