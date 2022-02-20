/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/piece/piece_data.hpp"
#include "primitives/seal_tasks/task.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/sector_file/sector_file.hpp"
#include "proofs/proof_engine.hpp"
#include "sector_storage/stores/store.hpp"
#include "sector_storage/stores/store_error.hpp"

namespace fc::api {
  struct StorageMinerApi;
}  // namespace fc::api

namespace fc::sector_storage {
  using primitives::piece::PieceData;
  using primitives::piece::PieceInfo;
  using primitives::piece::UnpaddedByteIndex;
  using primitives::piece::UnpaddedPieceSize;
  using primitives::sector::InteractiveRandomness;
  using primitives::sector::Proof;
  using primitives::sector::SealRandomness;
  using primitives::sector::SectorId;
  using primitives::sector::SectorRef;
  using primitives::sector_file::SectorFileType;
  using stores::AcquireMode;
  using stores::PathType;
  using stores::StoreError;
  using PreCommit1Output = proofs::Phase1Output;
  using Commit1Output = proofs::Phase1Output;
  using Update1Output = proofs::UpdateProofs1;
  using SectorCids = proofs::SealedAndUnsealedCID;
  using ReplicaUpdateOut = proofs::SealedAndUnsealedCID;

  struct Range {
    UnpaddedPieceSize offset;
    UnpaddedPieceSize size;
  };

  inline bool operator==(const Range &lhs, const Range &rhs) {
    return lhs.offset == rhs.offset && lhs.size == rhs.size;
  }

  struct CallId {
    SectorId sector{};
    std::string id;  // uuid
  };
  CBOR_TUPLE(CallId, sector, id);

  inline bool operator<(const CallId &lhs, const CallId &rhs) {
    return less(lhs.sector, rhs.sector, lhs.id, rhs.id);
  }

  inline bool operator==(const CallId &lhs, const CallId &rhs) {
    return lhs.sector == rhs.sector && lhs.id == rhs.id;
  }

  class WorkerCalls {
   public:
    virtual ~WorkerCalls() = default;

    virtual outcome::result<CallId> addPiece(
        const SectorRef &sector,
        gsl::span<const UnpaddedPieceSize> piece_sizes,
        const UnpaddedPieceSize &new_piece_size,
        PieceData piece_data) = 0;

    virtual outcome::result<CallId> sealPreCommit1(
        const SectorRef &sector,
        const SealRandomness &ticket,
        const std::vector<PieceInfo> &pieces) = 0;

    virtual outcome::result<CallId> sealPreCommit2(
        const SectorRef &sector,
        const PreCommit1Output &pre_commit_1_output) = 0;

    virtual outcome::result<CallId> sealCommit1(
        const SectorRef &sector,
        const SealRandomness &ticket,
        const InteractiveRandomness &seed,
        const std::vector<PieceInfo> &pieces,
        const SectorCids &cids) = 0;

    virtual outcome::result<CallId> sealCommit2(
        const SectorRef &sector, const Commit1Output &commit_1_output) = 0;

    virtual outcome::result<CallId> finalizeSector(
        const SectorRef &sector,
        const gsl::span<const Range> &keep_unsealed) = 0;

    virtual outcome::result<CallId> replicaUpdate(
        const SectorRef &sector, const std::vector<PieceInfo> &pieces) = 0;

    virtual outcome::result<CallId> proveReplicaUpdate1(
        const SectorRef &sector,
        const CID &sector_key,
        const CID &new_sealed,
        const CID &new_unsealed) = 0;

    virtual outcome::result<CallId> proveReplicaUpdate2(
        const SectorRef &sector,
        const CID &sector_key,
        const CID &new_sealed,
        const CID &new_unsealed,
        const Update1Output &update_1_output) = 0;

    virtual outcome::result<CallId> moveStorage(const SectorRef &sector,
                                                SectorFileType types) = 0;

    virtual outcome::result<CallId> unsealPiece(
        const SectorRef &sector,
        UnpaddedByteIndex offset,
        const UnpaddedPieceSize &size,
        const SealRandomness &randomness,
        const CID &unsealed_cid) = 0;

    virtual outcome::result<CallId> readPiece(
        PieceData output,
        const SectorRef &sector,
        UnpaddedByteIndex offset,
        const UnpaddedPieceSize &size) = 0;

    virtual outcome::result<CallId> fetch(const SectorRef &sector,
                                          const SectorFileType &file_type,
                                          PathType path_type,
                                          AcquireMode mode) = 0;
  };

  class Worker : public WorkerCalls {
   public:
    virtual outcome::result<primitives::WorkerInfo> getInfo() = 0;

    virtual outcome::result<std::set<primitives::TaskType>>
    getSupportedTask() = 0;

    virtual outcome::result<std::vector<primitives::StoragePath>>
    getAccessiblePaths() = 0;

    virtual void ping(std::function<void(bool resp)> cb) = 0;
  };

  enum class CallErrorCode : uint64_t {
    kUnknown = 0,
    kWorkerRestart = 101,  // from lotus
    kAllocateSpace         // same as StoreError::kCannotReserve
  };

  struct CallError {
    CallErrorCode code;
    std::string message;
  };

  struct CallResult {
    // `Bytes` = (`Proof` | `PreCommit1Output` | `Commit1Output`)
    // 'SealedAndUnsealedCID' = ('SectorCids' | 'ReplicaUpdateOut)
    std::variant<std::monostate,
                 PieceInfo,
                 Bytes,
                 bool,
                 Update1Output,
                 proofs::SealedAndUnsealedCID>
        value;
    boost::optional<CallError> maybe_error;
  };
  using ReturnCb = std::function<void(outcome::result<CallResult>)>;

  using WorkerReturn = api::StorageMinerApi;

  enum class WorkerErrors {
    kCannotCreateSealedFile = 1,
    kCannotCreateCacheDir,
    kCannotRemoveCacheDir,
    kPiecesDoNotMatchSectorSize,
    kCannotCreateTempFile,
    kCannotGetNumberOfCPUs,
    kCannotGetVMStat,
    kCannotGetPageSize,
    kCannotOpenMemInfoFile,
    kCannotRemoveSector,
    kUnsupportedPlatform,
    kOutOfBound,
    kCannotOpenFile,
    kUnsupportedCall,
  };
}  // namespace fc::sector_storage

OUTCOME_HPP_DECLARE_ERROR(fc::sector_storage, WorkerErrors);
