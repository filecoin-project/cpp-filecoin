/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTOR_INDEX_HPP
#define CPP_FILECOIN_CORE_SECTOR_INDEX_HPP

#include <chrono>
#include "common/outcome.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/sector_file/sector_file.hpp"
#include "primitives/types.hpp"

namespace fc::sector_storage::stores {
  class Lock {
   public:
    virtual ~Lock() = default;
  };

  using fc::primitives::sector::RegisteredProof;
  using fc::primitives::sector::SectorId;
  using fc::primitives::sector_file::SectorFileType;
  using primitives::FsStat;
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
  };

  inline bool operator==(const StorageInfo &lhs, const StorageInfo &rhs) {
    return lhs.id == rhs.id && lhs.urls == rhs.urls && lhs.weight == rhs.weight
           && lhs.can_seal == rhs.can_seal && lhs.can_store == rhs.can_store;
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
        const SectorFileType &file_type) = 0;

    virtual outcome::result<void> storageDropSector(
        const StorageID &storage_id,
        const SectorId &sector,
        const SectorFileType &file_type) = 0;

    virtual outcome::result<std::vector<StorageInfo>> storageFindSector(
        const SectorId &sector,
        const SectorFileType &file_type,
        bool allow_fetch) = 0;

    virtual outcome::result<std::vector<StorageInfo>> storageBestAlloc(
        const SectorFileType &allocate,
        RegisteredProof seal_proof_type,
        bool sealing) = 0;

    virtual outcome::result<std::unique_ptr<Lock>> storageLock(
        const SectorId &sector, SectorFileType read, SectorFileType write) = 0;

    virtual std::unique_ptr<Lock> storageTryLock(const SectorId &sector,
                                                 SectorFileType read,
                                                 SectorFileType write) = 0;
  };

  enum class IndexErrors {
    kStorageNotFound = 1,
    kNoSuitableCandidate,
    kInvalidUrl,
    kCannotLockStorage,
  };
}  // namespace fc::sector_storage::stores

OUTCOME_HPP_DECLARE_ERROR(fc::sector_storage::stores, IndexErrors);

#endif  // CPP_FILECOIN_CORE_SECTOR_INDEX_HPP
