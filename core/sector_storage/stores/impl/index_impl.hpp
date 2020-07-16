/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_INDEX_IMPL_HPP
#define CPP_FILECOIN_INDEX_IMPL_HPP

#include "sector_storage/stores/index.hpp"

#include <shared_mutex>
#include <unordered_map>
#include "sector_storage/stores/impl/index_lock.hpp"

namespace fc::sector_storage::stores {

  struct StorageEntry {
    StorageInfo info;
    FsStat fs_stat;

    system_clock::time_point last_heartbeat;
    boost::optional<std::string> error;
  };

  class SectorIndexImpl : public SectorIndex {
   public:
    outcome::result<void> storageAttach(const StorageInfo &storage_info,
                                        const FsStat &stat) override;

    outcome::result<StorageInfo> getStorageInfo(
        const StorageID &storage_id) const override;

    outcome::result<void> storageReportHealth(
        const StorageID &storage_id, const HealthReport &report) override;

    outcome::result<void> storageDeclareSector(
        const StorageID &storage_id,
        const SectorId &sector,
        const SectorFileType &file_type) override;

    outcome::result<void> storageDropSector(
        const StorageID &storage_id,
        const SectorId &sector,
        const SectorFileType &file_type) override;

    outcome::result<std::vector<StorageInfo>> storageFindSector(
        const SectorId &sector,
        const SectorFileType &file_type,
        bool allow_fetch) override;

    outcome::result<std::vector<StorageInfo>> storageBestAlloc(
        const SectorFileType &allocate,
        RegisteredProof seal_proof_type,
        bool sealing) override;

    outcome::result<std::unique_ptr<Lock>> storageLock(
        const SectorId &sector,
        SectorFileType read,
        SectorFileType write) override;

    std::unique_ptr<Lock> storageTryLock(const SectorId &sector,
                                         SectorFileType read,
                                         SectorFileType write) override;

   private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<StorageID, StorageEntry> stores_;
    std::unordered_map<std::string, std::vector<StorageID>> sectors_;
    std::shared_ptr<IndexLock> index_lock_ = std::make_shared<IndexLock>();
  };
}  // namespace fc::sector_storage::stores

#endif  // CPP_FILECOIN_INDEX_IMPL_HPP
