/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sector_storage/stores/index.hpp"

namespace fc::sector_storage::stores {

  class RemoteSectorIndexImpl : public SectorIndex {
  public:
      outcome::result<void> storageAttach(const StorageInfo &storage_info, const FsStat &stat) override;

      outcome::result<StorageInfo> getStorageInfo(const StorageID &storage_id) const override;

      outcome::result<void> storageReportHealth(const StorageID &storage_id, const HealthReport &report) override;

      outcome::result<void>
      storageDeclareSector(const StorageID &storage_id, const SectorId &sector, const SectorFileType &file_type,
                           bool primary) override;

      outcome::result<void>
      storageDropSector(const StorageID &storage_id, const SectorId &sector, const SectorFileType &file_type) override;

      outcome::result<std::vector<StorageInfo>>
      storageFindSector(const SectorId &sector, const SectorFileType &file_type,
                        boost::optional<RegisteredSealProof> fetch_seal_proof_type) override;

      outcome::result<std::vector<StorageInfo>>
      storageBestAlloc(const SectorFileType &allocate, RegisteredSealProof seal_proof_type, bool sealing_mode) override;

      outcome::result<std::unique_ptr<Lock>>
      storageLock(const SectorId &sector, SectorFileType read, SectorFileType write) override;

      std::unique_ptr<Lock> storageTryLock(const SectorId &sector, SectorFileType read, SectorFileType write) override;

  };
}  // namespace fc::sector_storage
