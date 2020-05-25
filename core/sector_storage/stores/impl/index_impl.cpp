/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/stores/impl/index_impl.hpp"
#include "primitives/types.hpp"

namespace fc::sector_storage::stores {

  using fc::primitives::TokenAmount;
  using std::chrono::system_clock;

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
        .info = storage_info,
        .fs_stat = stat,
        .last_heartbreak = system_clock::now(),
        .error = {},
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

  outcome::result<void> SectorIndexImpl::storageReportHealth(
      const ID &storage_id, const HealthReport &report) {
    std::unique_lock lock(mutex_);
    auto storage_iter = stores_.find(storage_id);
    if (storage_iter == stores_.end()) return IndexErrors::StorageNotFound;

    storage_iter->second.fs_stat = report.stat;
    storage_iter->second.error = report.error;
    storage_iter->second.last_heartbreak = system_clock::now();

    return outcome::success();
  }

  outcome::result<void> SectorIndexImpl::storageDeclareSector(
      const ID &storage_id,
      const SectorId &sector,
      const fc::primitives::sector_file::SectorFileType &file_type) {
    return outcome::success();
  }

  outcome::result<void> SectorIndexImpl::storageDropSector(
      const ID &storage_id,
      const SectorId &sector,
      const fc::primitives::sector_file::SectorFileType &file_type) {
    return outcome::success();
  }

  outcome::result<std::vector<StorageInfo>> SectorIndexImpl::storageFindSector(
      const SectorId &sector,
      const fc::primitives::sector_file::SectorFileType &file_type,
      bool allow_fetch) {
    return outcome::success();
  }

  outcome::result<std::vector<StorageInfo>> SectorIndexImpl::storageBestAlloc(
      const fc::primitives::sector_file::SectorFileType &allocate,
      fc::primitives::sector::RegisteredProof seal_proof_type,
      bool sealing) {
    std::shared_lock lock(mutex_);

    OUTCOME_TRY(
        req_space,
        fc::primitives::sector_file::sealSpaceUse(allocate, seal_proof_type));

    std::vector<StorageEntry> candidates;

    for (const auto &[id, storage] : stores_) {
      if (sealing && !storage.info.can_seal) {
        continue;
      }
      if (!sealing && !storage.info.can_store) {
        continue;
      }

      if (req_space > storage.fs_stat.available) {
        continue;
      }

      if (system_clock::now() - storage.last_heartbreak
          > kSkippedHeartbeatThreshold) {
        continue;
      }

      if (storage.error) {
        continue;
      }

      candidates.push_back(storage);
    }

    if (candidates.empty()) {
      return IndexErrors::NoSuitableCandidate;
    }

    std::sort(candidates.begin(),
              candidates.end(),
              [](const StorageEntry &lhs, const StorageEntry &rhs) {
                auto lw = TokenAmount(lhs.fs_stat.available)
                          * TokenAmount(lhs.info.weight);
                auto rw = TokenAmount(rhs.fs_stat.available)
                          * TokenAmount(rhs.info.weight);
                return lw > rw;
              });

    std::vector<StorageInfo> result;

    for (const auto &candidate : candidates) {
      result.emplace_back(candidate.info);
    }

    return result;
  }

}  // namespace fc::sector_storage::stores

OUTCOME_CPP_DEFINE_CATEGORY(fc::sector_storage::stores, IndexErrors, e) {
  using fc::sector_storage::stores::IndexErrors;
  switch (e) {
    case (IndexErrors::StorageNotFound):
      return "Sector Index: storage by ID not found";
    case (IndexErrors::NoSuitableCandidate):
      return "Sector Index: not found a suitable storage";
    default:
      return "Sector Index: unknown error";
  }
}
