/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_INDEX_IMPL_HPP
#define CPP_FILECOIN_INDEX_IMPL_HPP

#include "sector_storage/stores/index.hpp"

#include <unordered_map>
#include <shared_mutex>

namespace fc::sector_storage::stores {

    struct StorageEntry{
        StorageInfo info;
        FsStat file_stat;

        // TODO: add heartbeat
    };

    class SectorIndexImpl : SectorIndex {
    public:
        outcome::result<void> storageAttach(const StorageInfo &storage_info, const FsStat &stat) override;

        outcome::result<StorageInfo> getStorageInfo(const ID &storage_id) const override;

        outcome::result<void> StorageReportHealth(const ID &storage_id, const HealthReport &report) override;

        outcome::result<void>
        StorageDeclareSector(const ID &storage_id, const SectorId &sector, const SectorFileType &file_type) override;

        outcome::result<void>
        StorageDropSector(const ID &storage_id, const SectorId &sector, const SectorFileType &file_type) override;

        outcome::result<std::vector<StorageInfo>>
        StorageFindSector(const SectorId &sector, const SectorFileType &file_type, bool allow_fetch) override;

        outcome::result<std::vector<StorageInfo>>
        StorageBestAlloc(const SectorFileType &allocate, RegisteredProof seal_proof_type, bool sealing) override;

    private:
        mutable std::shared_mutex mutex_;
        std::unordered_map<ID, StorageEntry> stores_;
    };
}

#endif //CPP_FILECOIN_INDEX_IMPL_HPP
