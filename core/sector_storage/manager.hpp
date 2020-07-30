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

using fc::primitives::FsStat;
using fc::primitives::SectorSize;
using fc::primitives::StorageID;
using fc::primitives::piece::PieceData;
using fc::primitives::piece::UnpaddedByteIndex;

namespace fc::sector_storage {
  class Manager : public Sealer,
                  public Storage,
                  public Prover,
                  public FaultTracker {
   public:
    virtual SectorSize getSectorSize() = 0;

    virtual outcome::result<void> ReadPiece(proofs::PieceData output,
                                            const SectorId &sector,
                                            UnpaddedByteIndex offset,
                                            const UnpaddedPieceSize &size,
                                            const SealRandomness &randomness,
                                            const CID &cid) = 0;

    virtual outcome::result<void> addLocalStorage(const std::string &path) = 0;

    virtual outcome::result<void> addWorker(std::shared_ptr<Worker> worker) = 0;

    virtual outcome::result<std::unordered_map<StorageID, std::string>>
    getLocalStorages() = 0;

    virtual outcome::result<FsStat> getFsStat(StorageID storage_id) = 0;
  };

  enum class ManagerErrors {
    kCannotGetHomeDir = 1,
    kSomeSectorSkipped,
  };
}  // namespace fc::sector_storage

OUTCOME_HPP_DECLARE_ERROR(fc::sector_storage, ManagerErrors);

#endif  // CPP_FILECOIN_CORE_SECTOR_STORAGE_MANAGER_HPP
