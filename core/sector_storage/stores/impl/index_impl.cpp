/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/stores/impl/index_impl.hpp"

namespace fc::sector_storage::stores {

  outcome::result<void> SectorIndexImpl::storageAttach(
      const StorageInfo &storage_info, const FsStat &stat) {
    std::unique_lock lock(mutex_);
    // TODO(artyom-yurin): check validity of urls

    auto stores_iter = stores_.find(storage_info.id);
    if (stores_iter != stores_.end()) {
      bool is_duplicate;
      for (const auto &new_url : storage_info.urls) {
        is_duplicate = false;
        for (const auto &old_url : stores_iter->second.info.urls) {
          if (new_url == old_url) {
            is_duplicate = true;
            break;
          }
        }
        if (!is_duplicate) {
          stores_iter->second.info.urls.push_back(new_url);
        }
      }
      return outcome::success();
    }

    stores_[storage_info.id] = StorageEntry{
        .info = storage_info, .file_stat = stat,
        // HeartBeat - now
    };
    return outcome::success();
  }

  outcome::result<StorageInfo> SectorIndexImpl::getStorageInfo(
      const ID &storage_id) const {
    std::shared_lock lock(mutex_);
    auto maybe_storage = stores_.find(storage_id);
    if (maybe_storage == stores_.end()) return IndexErrors::StorageNotFound;
    return maybe_storage->second.info;
  }

  outcome::result<void> SectorIndexImpl::StorageReportHealth(
      const ID &storage_id, const HealthReport &report) {
    return outcome::success();
  }

  outcome::result<void> SectorIndexImpl::StorageDeclareSector(
      const ID &storage_id,
      const SectorId &sector,
      const fc::primitives::sector_file::SectorFileType &file_type) {
    return outcome::success();
  }

  outcome::result<void> SectorIndexImpl::StorageDropSector(
      const ID &storage_id,
      const SectorId &sector,
      const fc::primitives::sector_file::SectorFileType &file_type) {
    return outcome::success();
  }

  outcome::result<std::vector<StorageInfo>> SectorIndexImpl::StorageFindSector(
      const SectorId &sector,
      const fc::primitives::sector_file::SectorFileType &file_type,
      bool allow_fetch) {
    return outcome::success();
  }

  outcome::result<std::vector<StorageInfo>> SectorIndexImpl::StorageBestAlloc(
      const fc::primitives::sector_file::SectorFileType &allocate,
      fc::primitives::sector::RegisteredProof seal_proof_type,
      bool sealing) {
    return outcome::success();
  }

}  // namespace fc::sector_storage::stores

OUTCOME_CPP_DEFINE_CATEGORY(fc::sector_storage::stores, IndexErrors, e) {
  using fc::sector_storage::stores::IndexErrors;
  switch (e) {
    case (IndexErrors::StorageNotFound):
      return "Sector Index: storage by ID not found";
    default:
      return "Sector: unknown error";
  }
}
