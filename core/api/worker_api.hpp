/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/utils.hpp"
#include "api/version.hpp"
#include "common/outcome.hpp"
#include "sector_storage/worker.hpp"

namespace fc::api {
  using primitives::piece::PieceInfo;
  using primitives::piece::UnpaddedByteIndex;
  using primitives::piece::UnpaddedPieceSize;
  using primitives::sector::InteractiveRandomness;
  using primitives::sector::SealRandomness;
  using primitives::sector::SectorId;
  using sector_storage::AcquireMode;
  using sector_storage::Commit1Output;
  using sector_storage::PathType;
  using sector_storage::PreCommit1Output;
  using sector_storage::Proof;
  using sector_storage::Range;
  using sector_storage::SectorCids;
  using sector_storage::SectorFileType;

  struct WorkerApi {
    // TODO(ortyomka): [FIL-344] add AddPiece function

    API_METHOD(Fetch,
               void,
               const SectorId &,
               const SectorFileType &,
               PathType,
               AcquireMode)
    API_METHOD(FinalizeSector,
               void,
               const SectorId &,
               const std::vector<Range> &)

    API_METHOD(Info, primitives::WorkerInfo)

    API_METHOD(MoveStorage, void, const SectorId &)

    API_METHOD(Paths, std::vector<primitives::StoragePath>)
    API_METHOD(Remove, void, const SectorId &)

    API_METHOD(SealCommit1,
               Commit1Output,
               const SectorId &,
               const SealRandomness &,
               const InteractiveRandomness &,
               std::vector<PieceInfo>,
               const SectorCids &)

    API_METHOD(SealCommit2, Proof, const SectorId &, const Commit1Output &)

    API_METHOD(SealPreCommit1,
               PreCommit1Output,
               const SectorId &,
               const SealRandomness &,
               std::vector<PieceInfo>)

    API_METHOD(SealPreCommit2,
               SectorCids,
               const SectorId &,
               const PreCommit1Output &)

    API_METHOD(StorageAddLocal, void, const std::string &)

    API_METHOD(TaskTypes, std::set<primitives::TaskType>)

    API_METHOD(UnsealPiece,
               void,
               const SectorId &,
               UnpaddedByteIndex,
               const UnpaddedPieceSize &,
               const SealRandomness &,
               const CID &)

    API_METHOD(Version, VersionResult)
  };

  template <typename A, typename F>
  void visit(const WorkerApi &, A &&a, const F &f) {
    f(a.Fetch);
    f(a.FinalizeSector);
    f(a.Info);
    f(a.MoveStorage);
    f(a.Paths);
    f(a.Remove);
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
