/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/stores/impl/index_impl.hpp"

#include <boost/filesystem/path.hpp>
#include <regex>
#include "common/uri_parser/uri_parser.hpp"
#include "primitives/types.hpp"

namespace fc::sector_storage::stores {

  using fc::common::HttpUri;
  using fc::primitives::TokenAmount;
  using primitives::sector_file::sectorName;
  using std::chrono::system_clock;

  std::string toSectorsID(const SectorId &sector_id,
                          const SectorFileType &file_type) {
    return sectorName(sector_id) + "_" + toString(file_type);
  }

  bool isValidUrl(const std::string &url) {
    static std::regex url_regex(
        "https?:\\/\\/"
        "(www\\.)?[-a-zA-Z0-9@:%._\\+~#=]{2,256}\\.[a-z]{2,4}\\b([-a-zA-Z0-9@:%"
        "_\\+.~#?&//=]*)");
    return std::regex_match(url, url_regex);
  }

  outcome::result<void> SectorIndexImpl::storageAttach(
      const StorageInfo &storage_info, const FsStat &stat) {
    std::unique_lock lock(mutex_);
    for (const auto &new_url : storage_info.urls) {
      if (!isValidUrl(new_url)) {
        return IndexErrors::kInvalidUrl;
      }
    }

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
        .last_heartbeat = system_clock::now(),
        .error = {},
    };
    return outcome::success();
  }

  outcome::result<StorageInfo> SectorIndexImpl::getStorageInfo(
      const StorageID &storage_id) const {
    std::shared_lock lock(mutex_);
    auto maybe_storage = stores_.find(storage_id);
    if (maybe_storage == stores_.end()) return IndexErrors::kStorageNotFound;
    return maybe_storage->second.info;
  }

  outcome::result<void> SectorIndexImpl::storageReportHealth(
      const StorageID &storage_id, const HealthReport &report) {
    std::unique_lock lock(mutex_);
    auto storage_iter = stores_.find(storage_id);
    if (storage_iter == stores_.end()) return IndexErrors::kStorageNotFound;

    storage_iter->second.fs_stat = report.stat;
    storage_iter->second.error = report.error;
    storage_iter->second.last_heartbeat = system_clock::now();

    return outcome::success();
  }

  outcome::result<void> SectorIndexImpl::storageDeclareSector(
      const StorageID &storage_id,
      const SectorId &sector,
      const SectorFileType &file_type) {
    std::unique_lock lock(mutex_);

    for (const auto &type : primitives::sector_file::kSectorFileTypes) {
      if ((file_type & type) == 0) {
        continue;
      }

      std::string sector_id = toSectorsID(sector, type);

      auto sector_iter = sectors_.find(sector_id);
      if (sector_iter == sectors_.end()) {
        sectors_[sector_id] = {};
        sector_iter = sectors_.find(sector_id);
      }
      for (const auto &s_id : sector_iter->second) {
        if (storage_id == s_id) {
          // already there
          return outcome::success();
        }
      }

      sector_iter->second.push_back(storage_id);
    }

    return outcome::success();
  }

  outcome::result<void> SectorIndexImpl::storageDropSector(
      const StorageID &storage_id,
      const SectorId &sector,
      const fc::primitives::sector_file::SectorFileType &file_type) {
    std::unique_lock lock(mutex_);

    for (const auto &type : primitives::sector_file::kSectorFileTypes) {
      if ((file_type & type) == 0) {
        continue;
      }

      std::string sector_id = toSectorsID(sector, type);

      auto sector_iter = sectors_.find(sector_id);
      if (sector_iter == sectors_.end()) {
        return outcome::success();
      }
      std::vector<StorageID> sectors;
      for (const auto &s_id : sector_iter->second) {
        if (storage_id == s_id) {
          continue;
        }
        sectors.push_back(s_id);
      }
      if (sectors.size() == 0) {
        sectors_.erase(sector_iter);
        return outcome::success();
      }

      sector_iter->second = sectors;
    }

    return outcome::success();
  }

  outcome::result<std::vector<StorageInfo>> SectorIndexImpl::storageFindSector(
      const SectorId &sector,
      const fc::primitives::sector_file::SectorFileType &file_type,
      bool allow_fetch) {
    std::shared_lock lock(mutex_);
    std::unordered_map<StorageID, uint64_t> storage_ids;

    for (const auto &type : primitives::sector_file::kSectorFileTypes) {
      if ((file_type & type) == 0) {
        continue;
      }

      std::string sector_id = toSectorsID(sector, type);
      auto sector_iter = sectors_.find(sector_id);
      if (sector_iter == sectors_.end()) {
        continue;
      }
      for (const auto &id : sector_iter->second) {
        ++storage_ids[id];
      }
    }

    std::vector<StorageInfo> result;
    for (const auto &[id, count] : storage_ids) {
      if (stores_.find(id) == stores_.end()) {
        // logger
        continue;
      }

      auto store = stores_[id].info;

      for (uint64_t i = 0; i < store.urls.size(); i++) {
        HttpUri uri;
        try {
          uri.parse(store.urls[i]);
        } catch (const std::runtime_error &err) {
          return IndexErrors::kInvalidUrl;
        }
        boost::filesystem::path path = uri.path();
        path = path / toString(file_type) / sectorName(sector);
        uri.setPath(path.string());
        store.urls[i] = uri.str();
      }

      store.weight = store.weight * count;
      result.push_back(store);
    }

    if (allow_fetch) {
      for (const auto &[id, storage_info] : stores_) {
        if (storage_ids.find(id) != storage_ids.end()) {
          continue;
        }
        auto store = stores_[id].info;

        for (uint64_t i = 0; i < store.urls.size(); i++) {
          HttpUri uri;
          try {
            uri.parse(store.urls[i]);
          } catch (const std::runtime_error &err) {
            return IndexErrors::kInvalidUrl;
          }
          boost::filesystem::path path = uri.path();
          path = path / toString(file_type) / sectorName(sector);
          uri.setPath(path.string());
          store.urls[i] = uri.str();
        }

        store.weight = 0;
        result.push_back(store);
      }
    }

    return result;
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

      if (system_clock::now() - storage.last_heartbeat
          > kSkippedHeartbeatThreshold) {
        continue;
      }

      if (storage.error) {
        continue;
      }

      candidates.push_back(storage);
    }

    if (candidates.empty()) {
      return IndexErrors::kNoSuitableCandidate;
    }

    std::sort(candidates.begin(),
              candidates.end(),
              [](const StorageEntry &lhs, const StorageEntry &rhs) {
                auto lw = TokenAmount(lhs.fs_stat.available) * lhs.info.weight;
                auto rw = TokenAmount(rhs.fs_stat.available) * rhs.info.weight;
                return lw < rw;
              });

    std::vector<StorageInfo> result;

    for (const auto &candidate : candidates) {
      result.emplace_back(candidate.info);
    }

    return result;
  }

  outcome::result<std::unique_ptr<Lock>> SectorIndexImpl::storageLock(
      const SectorId &sector, SectorFileType read, SectorFileType write) {
    std::unique_ptr<IndexLock::Lock> lock =
        std::make_unique<IndexLock::Lock>(sector, read, write);

    if (lock) {
      auto is_locked = index_lock_->lock(*lock, true);

      if (is_locked) {
        return std::move(lock);
      }
    }

    return IndexErrors::kCannotLockStorage;
  }

  std::unique_ptr<Lock> SectorIndexImpl::storageTryLock(const SectorId &sector,
                                                        SectorFileType read,
                                                        SectorFileType write) {
    auto lock = std::make_unique<IndexLock::Lock>(sector, read, write);

    auto is_locked = index_lock_->lock(*lock, false);

    if (is_locked) {
      return std::move(lock);
    }

    return nullptr;
  }

}  // namespace fc::sector_storage::stores

OUTCOME_CPP_DEFINE_CATEGORY(fc::sector_storage::stores, IndexErrors, e) {
  using fc::sector_storage::stores::IndexErrors;
  switch (e) {
    case (IndexErrors::kStorageNotFound):
      return "Sector Index: storage by ID not found";
    case (IndexErrors::kNoSuitableCandidate):
      return "Sector Index: not found a suitable storage";
    case (IndexErrors::kInvalidUrl):
      return "Sector Index: failed to parse url";
    case (IndexErrors::kCannotLockStorage):
      return "Sector Index: failed to acquire lock";
    default:
      return "Sector Index: unknown error";
  }
}
