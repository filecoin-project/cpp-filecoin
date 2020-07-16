/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTOR_STORAGE_MANAGER_HPP
#define CPP_FILECOIN_CORE_SECTOR_STORAGE_MANAGER_HPP

#include "common/outcome.hpp"
#include "sector_storage/fault_tracker.hpp"
#include "sector_storage/spec_interfaces/prover.hpp"
#include "sector_storage/spec_interfaces/sealer.hpp"
#include "sector_storage/spec_interfaces/storage.hpp"
#include "sector_storage/worker.hpp"

using fc::primitives::SectorSize;
using fc::primitives::piece::PieceData;
using fc::primitives::piece::UnpaddedByteIndex;
using fc::primitives::StorageID;
using fc::primitives::FsStat;

namespace fc::sector_storage {
  class Manager : public Sealer, Storage, public Prover, FaultTracker {
   public:
    virtual SectorSize getSectorSize() = 0;

    virtual outcome::result<void> ReadPiece(const proofs::PieceData &output,
                                            const SectorId &sector,
                                            UnpaddedByteIndex offset,
                                            const UnpaddedPieceSize &size,
                                            const SealRandomness &randomness,
                                            const CID &cid) = 0;

    virtual outcome::result<void> addLocalStorage(const std::string &path) = 0;

    virtual outcome::result<void> addWorker(const Worker &worker) = 0;

    virtual outcome::result<std::unordered_map<StorageID, std::string>> getStorageLocal() = 0;

    virtual outcome::result<FsStat> getFsStat(StorageID storage_id) = 0;
  };
}  // namespace fc::sector_storage

#endif  // CPP_FILECOIN_CORE_SECTOR_STORAGE_MANAGER_HPP
