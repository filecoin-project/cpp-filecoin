/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sector_storage/stores/index.hpp"

#include <shared_mutex>
#include <unordered_map>
#include "common/logger.hpp"
#include "sector_storage/stores/impl/index_lock.hpp"

namespace fc::sector_storage::stores {

  struct StorageEntry {
    StorageInfo info;
    FsStat fs_stat;

    system_clock::time_point last_heartbeat;
    boost::optional<std::string> error;
  };

  struct Decl {
    SectorId sector_id;
    SectorFileType type;
  };

  inline bool operator<(const Decl &lhs, const Decl &rhs) {
    return less(lhs.sector_id, rhs.sector_id, lhs.type, rhs.type);
  }

  class SectorIndexImpl : public SectorIndex {
   public:
    SectorIndexImpl();

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
        boost::optional<SectorSize> fetch_sector_size) override;

    outcome::result<std::vector<StorageInfo>> storageBestAlloc(
        const SectorFileType &allocate,
        SectorSize sector_size,
        bool sealing_mode) override;

    outcome::result<std::unique_ptr<WLock>> storageLock(
        const SectorId &sector,
        SectorFileType read,
        SectorFileType write) override;

    std::unique_ptr<WLock> storageTryLock(const SectorId &sector,
                                          SectorFileType read,
                                          SectorFileType write) override;

   private:
    struct DeclMeta {
      StorageID id;
      bool is_primary;
    };

    mutable std::shared_mutex mutex_;
    std::unordered_map<StorageID, StorageEntry> stores_;
    std::map<Decl, std::vector<DeclMeta>> sectors_;
    std::shared_ptr<IndexLock> index_lock_;
    common::Logger logger_;
  };
}  // namespace fc::sector_storage::stores
