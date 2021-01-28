/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "sector_storage/worker.hpp"

#define API_METHOD(_name, _result, ...)                                    \
  struct _##_name : std::function<outcome::result<_result>(__VA_ARGS__)> { \
    using function::function;                                              \
    using Result = _result;                                                \
    using Params = ParamsTuple<__VA_ARGS__>;                               \
    static constexpr auto name = "Filecoin." #_name;                       \
  } _name;

namespace fc::api {
  template <typename... T>
  using ParamsTuple =
      std::tuple<std::remove_const_t<std::remove_reference_t<T>>...>;

  using primitives::piece::PieceInfo;
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
               const std::vector<const Range> &)

    API_METHOD(Info, primitives::WorkerInfo, void)

    API_METHOD(MoveStorage, void, const SectorId &)

    API_METHOD(Paths, std::vector<primitives::StoragePath>, void)
    API_METHOD(Remove, void, const SectorId &)

    API_METHOD(SealCommit1,
               Commit1Output,
               const SectorId &,
               const SealRandomness &,
               const InteractiveRandomness &,
               std::vector<const PieceInfo>,
               const SectorCids &)

    API_METHOD(SealCommit2, Proof, const SectorId &, const Commit1Output &)

    API_METHOD(SealPreCommit1,
               PreCommit1Output,
               const SectorId &,
               const SealRandomness &,
               std::vector<const PieceInfo>)

    API_METHOD(SealPreCommit2,
               SectorCids,
               const SectorId &,
               const PreCommit1Output &)

    API_METHOD(StorageAddLocal, void, const std::string &)

    API_METHOD(TaskTypes, std::set<primitives::TaskType>, void)

    API_METHOD(UnsealPiece,
               void,
               const SectorId &,
               UnpaddedByteIndex,
               const UnpaddedPieceSize &,
               const SealRandomness &,
               const CID &)

    API_METHOD(Version, uint64_t, void)
  };
}  // namespace fc::api
