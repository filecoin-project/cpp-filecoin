/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTOR_INDEX_HPP
#define CPP_FILECOIN_CORE_SECTOR_INDEX_HPP

#include "primitives/sector/sector.hpp"
#include "primitives/sector_file/sector_file.hpp"
#include "common/outcome.hpp"

namespace fc::sector_storage::stores {
  // ID identifies sector storage by UUID. One sector storage should map to one
  //  filesystem, local or networked / shared by multiple machines
  using ID = std::string;
  using fc::primitives::sector::RegisteredProof;
  using fc::primitives::sector::SectorId;
  using fc::primitives::sector_file::SectorFileType;

  struct StorageInfo {
    ID id;
    std::vector<std::string> urls;  // TODO: Support non-http transports
    uint64_t weight;

    bool can_seal;
    bool can_store;

    // TODO: add heartbeat
  };

  struct FsStat {
    uint64_t capacity;
    uint64_t available;  // Available to use for sector storage
    uint64_t used;
  };

  // TODO: remove after integrate heartbeat
  struct HealthReport {
    FsStat stat;
    std::string err;
  };

  class SectorIndex {
   public:
    virtual ~SectorIndex() = default;

    virtual outcome::result<void> storageAttach(const StorageInfo &storage_info,
                                                const FsStat &stat) = 0;

    virtual outcome::result<StorageInfo> getStorageInfo(
        const ID &storage_id) const = 0;

    virtual outcome::result<void> StorageReportHealth(
        const ID &storage_id, const HealthReport &report) = 0;

    virtual outcome::result<void> StorageDeclareSector(
        const ID &storage_id,
        const SectorId &sector,
        const SectorFileType &file_type) = 0;

    virtual outcome::result<void> StorageDropSector(
        const ID &storage_id,
        const SectorId &sector,
        const SectorFileType &file_type) = 0;

    virtual outcome::result<std::vector<StorageInfo>> StorageFindSector(
        const SectorId &sector,
        const SectorFileType &file_type,
        bool allow_fetch) = 0;

    virtual outcome::result<std::vector<StorageInfo>> StorageBestAlloc(
        const SectorFileType &allocate,
        RegisteredProof seal_proof_type,
        bool sealing) = 0;
  };

    enum class IndexErrors {
        StorageNotFound = 1,
    };
}  // namespace fc::sector_storage::stores

OUTCOME_HPP_DECLARE_ERROR(fc::sector_storage::stores, IndexErrors);

#endif  // CPP_FILECOIN_CORE_SECTOR_INDEX_HPP
