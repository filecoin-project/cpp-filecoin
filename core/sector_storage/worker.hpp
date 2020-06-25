/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTOR_STORAGE_WORKER_HPP
#define CPP_FILECOIN_CORE_SECTOR_STORAGE_WORKER_HPP

#include "sector_storage/spec_interfaces/sealer.hpp"
#include "sector_storage/spec_interfaces/storage.hpp"

#include <set>
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
                                              UnpaddedByteIndex index,
                                              const UnpaddedPieceSize &size,
                                              const SealRandomness &randomness,
                                              const CID &cid) = 0;

    virtual outcome::result<void> readPiece(common::Buffer &output,
                                            const SectorId &sector,
                                            UnpaddedByteIndex index,
                                            const UnpaddedPieceSize &size) = 0;

    virtual outcome::result<primitives::WorkerInfo> getInfo() = 0;

    virtual outcome::result<std::vector<primitives::TaskType>>
    getSupportedTask() = 0;

    virtual outcome::result<std::vector<primitives::StoragePath>>
    getAccessiblePaths() = 0;

    // TODO(artyom-yurin): [FIL-222] think about how to close the worker
  };

}  // namespace fc::sector_storage

#endif  // CPP_FILECOIN_CORE_SECTOR_STORAGE_WORKER_HPP
