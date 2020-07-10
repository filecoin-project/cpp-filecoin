/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTOR_STORAGE_WORKER_HPP
#define CPP_FILECOIN_CORE_SECTOR_STORAGE_WORKER_HPP

#include "sector_storage/spec_interfaces/sealer.hpp"
#include "sector_storage/spec_interfaces/storage.hpp"

#include "common/outcome.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/seal_tasks/task.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/sector_file/sector_file.hpp"

namespace fc::sector_storage {

  using primitives::piece::UnpaddedByteIndex;
  using primitives::piece::UnpaddedPieceSize;
  using primitives::sector::SealRandomness;
  using primitives::sector::SectorId;
  using primitives::sector_file::SectorFileType;

  class Worker : public Sealer, Storage {
   public:
    virtual outcome::result<void> moveStorage(const SectorId &sector) = 0;

    virtual outcome::result<void> fetch(const SectorId &sector,
                                        const SectorFileType &file_type,
                                        bool can_seal) = 0;

    virtual outcome::result<void> unsealPiece(const SectorId &sector,
                                              UnpaddedByteIndex offset,
                                              const UnpaddedPieceSize &size,
                                              const SealRandomness &randomness,
                                              const CID &unsealed_cid) = 0;

    /**
     * @param output is PieceData with write part of pipe
     */
    virtual outcome::result<void> readPiece(const proofs::PieceData &output,
                                            const SectorId &sector,
                                            UnpaddedByteIndex offset,
                                            const UnpaddedPieceSize &size) = 0;

    virtual outcome::result<primitives::WorkerInfo> getInfo() = 0;

    virtual outcome::result<std::vector<primitives::TaskType>>
    getSupportedTask() = 0;

    virtual outcome::result<std::vector<primitives::StoragePath>>
    getAccessiblePaths() = 0;

    // TODO(artyom-yurin): [FIL-222] think about how to close the worker
  };

  enum class WorkerErrors {
    CANNOT_CREATE_SEALED_FILE = 1,
    CANNOT_CREATE_CACHE_DIR,
    CANNOT_REMOVE_CACHE_DIR,
    PIECES_DO_NOT_MATCH_SECTOR_SIZE,
    OUTPUT_DOES_NOT_OPEN,
    OUT_OF_BOUND_OF_FILE,
    CANNOT_OPEN_UNSEALED_FILE,
    CANNOT_GET_NUMBER_OF_CPUS,
    CANNOT_GET_VM_STAT,
    CANNOT_GET_PAGE_SIZE,
    CANNOT_OPEN_MEM_INFO_FILE,
    CANNOT_REMOVE_SECTOR,
    UNSUPPORTED_PLATFORM,
  };
}  // namespace fc::sector_storage

OUTCOME_HPP_DECLARE_ERROR(fc::sector_storage, WorkerErrors);

#endif  // CPP_FILECOIN_CORE_SECTOR_STORAGE_WORKER_HPP
