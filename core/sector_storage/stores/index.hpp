/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>
#include "common/outcome.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/sector_file/sector_file.hpp"
#include "primitives/types.hpp"

namespace fc::sector_storage::stores {
  using primitives::sector::RegisteredSealProof;

  class WLock {
   public:
    virtual ~WLock() = default;
  };

  using fc::primitives::sector::SectorId;
  using fc::primitives::sector_file::SectorFileType;
  using primitives::FsStat;
  using primitives::SectorSize;
  using primitives::StorageID;
  using std::chrono::system_clock;

  const std::chrono::seconds kHeartbeatInterval(10);
  const std::chrono::seconds kSkippedHeartbeatThreshold =
      kHeartbeatInterval * 5;

  struct StorageInfo {
    StorageID id;
    std::vector<std::string>
        urls;  // TODO (artyom-yurin): [FIL-200] Support non-http transports
    uint64_t weight;

    bool can_seal;
    bool can_store;

    bool is_primary;
  };

  inline bool operator==(const StorageInfo &lhs, const StorageInfo &rhs) {
    return lhs.id == rhs.id && lhs.urls == rhs.urls && lhs.weight == rhs.weight
           && lhs.can_seal == rhs.can_seal && lhs.can_store == rhs.can_store
           && lhs.is_primary == rhs.is_primary;
  }

  struct HealthReport {
    FsStat stat;
    boost::optional<std::string> error;
  };

  class SectorIndex {
   public:
    virtual ~SectorIndex() = default;

    virtual outcome::result<void> storageAttach(const StorageInfo &storage_info,
                                                const FsStat &stat) = 0;

    virtual outcome::result<StorageInfo> getStorageInfo(
        const StorageID &storage_id) const = 0;

    virtual outcome::result<void> storageReportHealth(
        const StorageID &storage_id, const HealthReport &report) = 0;

    virtual outcome::result<void> storageDeclareSector(
        const StorageID &storage_id,
        const SectorId &sector,
        const SectorFileType &file_type,
        bool primary) = 0;

    virtual outcome::result<void> storageDropSector(
        const StorageID &storage_id,
        const SectorId &sector,
        const SectorFileType &file_type) = 0;

    /**
     * @note to able to fetch, need to specify sector size
     */
    virtual outcome::result<std::vector<StorageInfo>> storageFindSector(
        const SectorId &sector,
        const SectorFileType &file_type,
        boost::optional<SectorSize> fetch_sector_size) = 0;

    virtual outcome::result<std::vector<StorageInfo>> storageBestAlloc(
        const SectorFileType &allocate, SectorSize sector_size, bool sealing_mode) = 0;

    virtual outcome::result<std::shared_ptr<WLock>> storageLock(
        const SectorId &sector, SectorFileType read, SectorFileType write) = 0;

    virtual std::shared_ptr<WLock> storageTryLock(const SectorId &sector,
                                                  SectorFileType read,
                                                  SectorFileType write) = 0;
  };

  enum class IndexErrors {
    kStorageNotFound = 1,
    kNoSuitableCandidate,
    kInvalidUrl,
    kCannotLockStorage,
    kNotSupportedMethod,
  };
}  // namespace fc::sector_storage::stores

OUTCOME_HPP_DECLARE_ERROR(fc::sector_storage::stores, IndexErrors);
