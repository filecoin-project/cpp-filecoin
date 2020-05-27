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

namespace fc::sector_storage::stores {
  // ID identifies sector storage by UUID. One sector storage should map to one
  //  filesystem, local or networked / shared by multiple machines
  using ID = std::string;
  using fc::primitives::sector::RegisteredProof;
  using fc::primitives::sector::SectorId;
  using fc::primitives::sector_file::SectorFileType;
  using std::chrono::system_clock;

  const std::chrono::seconds kHeartbeatInterval(10);
  const std::chrono::seconds kSkippedHeartbeatThreshold =
      kHeartbeatInterval * 5;

  struct StorageInfo {
    ID id;
    std::vector<std::string>
        urls;  // TODO (artyom-yurin): [FIL-200] Support non-http transports
    uint64_t weight;

    bool can_seal;
    bool can_store;

    system_clock::time_point last_heartbeat;
    boost::optional<std::string> error;
  };

  struct FsStat {
    uint64_t capacity;
    uint64_t available;  // Available to use for sector storage
    uint64_t used;
  };

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
        const ID &storage_id) const = 0;

    virtual outcome::result<void> storageReportHealth(
        const ID &storage_id, const HealthReport &report) = 0;

    virtual outcome::result<void> storageDeclareSector(
        const ID &storage_id,
        const SectorId &sector,
        const SectorFileType &file_type) = 0;

    virtual outcome::result<void> storageDropSector(
        const ID &storage_id,
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
  };

  enum class IndexErrors {
    StorageNotFound = 1,
    NoSuitableCandidate,
    InvalidUrl,
  };
}  // namespace fc::sector_storage::stores

OUTCOME_HPP_DECLARE_ERROR(fc::sector_storage::stores, IndexErrors);

#endif  // CPP_FILECOIN_CORE_SECTOR_INDEX_HPP
