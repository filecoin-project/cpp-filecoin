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

namespace fc::sector_storage {
  using primitives::piece::PieceData;
  using primitives::piece::PieceInfo;
  using primitives::piece::UnpaddedByteIndex;
  using primitives::piece::UnpaddedPieceSize;
  using primitives::sector::SealRandomness;
  using primitives::sector::SectorId;
  using primitives::sector::SectorRef;
  using primitives::sector_file::SectorFileType;
  using PreCommit1Output = proofs::Phase1Output;
  using Commit1Output = proofs::Phase1Output;
  using SectorCids = proofs::SealedAndUnsealedCID;
  using primitives::sector::InteractiveRandomness;
  using primitives::sector::Proof;
  using primitives::sector::SealRandomness;
  using stores::AcquireMode;
  using stores::PathType;
  using stores::StoreError;

  struct Range {
    UnpaddedPieceSize offset;
    UnpaddedPieceSize size;
  };

  inline bool operator==(const Range &lhs, const Range &rhs) {
    return lhs.offset == rhs.offset && lhs.size == rhs.size;
  }

  struct CallId {
    SectorId sector;
    std::string id;  // uuid
  };

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
        gsl::span<const PieceInfo> pieces) = 0;

    virtual outcome::result<CallId> sealPreCommit2(
        const SectorRef &sector,
        const PreCommit1Output &pre_commit_1_output) = 0;

    virtual outcome::result<CallId> sealCommit1(
        const SectorRef &sector,
        const SealRandomness &ticket,
        const InteractiveRandomness &seed,
        gsl::span<const PieceInfo> pieces,
        const SectorCids &cids) = 0;

    virtual outcome::result<CallId> sealCommit2(
        const SectorRef &sector, const Commit1Output &commit_1_output) = 0;

    virtual outcome::result<CallId> finalizeSector(
        const SectorRef &sector,
        const gsl::span<const Range> &keep_unsealed) = 0;

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

  template <typename T>
  boost::optional<CallError> toCallError(const outcome::result<T> &result) {
    if (not result.has_error()) {
      return boost::none;
    }

    CallError error{
        .code = CallErrorCode::kUnknown,
        .message = result.error().message(),
    };
    if (result == outcome::failure(StoreError::kCannotReserve)) {
      error.code = CallErrorCode::kAllocateSpace;
    }
    return error;
  }

  using ReturnValues =
      std::variant<PieceInfo,
                   SectorCids,
                   Proof,  // also PreCommit1Output, Commit1Output
                   bool>;
  struct CallResult {
    boost::optional<ReturnValues> maybe_value;
    boost::optional<CallError> maybe_error;
  };
  using ReturnCb = std::function<void(outcome::result<CallResult>)>;

  class WorkerReturn {
   public:
    virtual ~WorkerReturn() = default;

    virtual outcome::result<void> returnAddPiece(
        const CallId &call_id,
        const PieceInfo &maybe_piece_info,
        const boost::optional<CallError> &maybe_error) = 0;

    virtual outcome::result<void> returnSealPreCommit1(
        const CallId &call_id,
        const PreCommit1Output &maybe_precommit1_out,
        const boost::optional<CallError> &maybe_error) = 0;

    virtual outcome::result<void> returnSealPreCommit2(
        const CallId &call_id,
        const SectorCids &maybe_sector_cids,
        const boost::optional<CallError> &maybe_error) = 0;

    virtual outcome::result<void> returnSealCommit1(
        const CallId &call_id,
        const Commit1Output &maybe_commit1_out,
        const boost::optional<CallError> &maybe_error) = 0;

    virtual outcome::result<void> returnSealCommit2(
        const CallId &call_id,
        const Proof &maybe_proof,
        const boost::optional<CallError> &maybe_error) = 0;

    virtual outcome::result<void> returnFinalizeSector(
        const CallId &call_id,
        const boost::optional<CallError> &maybe_error) = 0;

    virtual outcome::result<void> returnMoveStorage(
        const CallId &call_id,
        const boost::optional<CallError> &maybe_error) = 0;

    virtual outcome::result<void> returnUnsealPiece(
        const CallId &call_id,
        const boost::optional<CallError> &maybe_error) = 0;

    virtual outcome::result<void> returnReadPiece(
        const CallId &call_id,
        bool maybe_status,
        const boost::optional<CallError> &maybe_error) = 0;

    virtual outcome::result<void> returnFetch(
        const CallId &call_id,
        const boost::optional<CallError> &maybe_error) = 0;
  };

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
