/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sector_storage/stores/index.hpp"

#include "api/storage_miner/storage_api.hpp"

namespace fc::sector_storage::stores {
  using api::StorageMinerApi;

  class RemoteSectorIndexImpl : public SectorIndex {
   public:
    RemoteSectorIndexImpl(std::shared_ptr<StorageMinerApi> api);

    outcome::result<void> storageAttach(const StorageInfo &storage_info,
                                        const FsStat &stat) override;

    outcome::result<StorageInfo> getStorageInfo(
        const StorageID &storage_id) const override;

    outcome::result<void> storageReportHealth(
        const StorageID &storage_id, const HealthReport &report) override;

    outcome::result<void> storageDeclareSector(const StorageID &storage_id,
                                               const SectorId &sector,
                                               const SectorFileType &file_type,
                                               bool primary) override;

    outcome::result<void> storageDropSector(
        const StorageID &storage_id,
        const SectorId &sector,
        const SectorFileType &file_type) override;

    outcome::result<std::vector<StorageInfo>> storageFindSector(
        const SectorId &sector,
        const SectorFileType &file_type,
        boost::optional<RegisteredSealProof> fetch_seal_proof_type) override;

    outcome::result<std::vector<StorageInfo>> storageBestAlloc(
        const SectorFileType &allocate,
        RegisteredSealProof seal_proof_type,
        bool sealing_mode) override;

    outcome::result<std::unique_ptr<WLock>> storageLock(
        const SectorId &sector,
        SectorFileType read,
        SectorFileType write) override;

    std::unique_ptr<WLock> storageTryLock(const SectorId &sector,
                                          SectorFileType read,
                                          SectorFileType write) override;

   private:
    std::shared_ptr<StorageMinerApi> api_;
  };
}  // namespace fc::sector_storage::stores
