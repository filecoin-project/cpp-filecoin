/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/stores/impl/index_impl.hpp"

#include <boost/filesystem/path.hpp>
#include <chrono>
#include <regex>
#include "common/uri_parser/uri_parser.hpp"
#include "primitives/types.hpp"

namespace fc::sector_storage::stores {

  using fc::common::HttpUri;
  using primitives::BigInt;
  using primitives::sector_file::sectorName;
  using std::chrono::duration_cast;
  using std::chrono::high_resolution_clock;
  using std::chrono::system_clock;

  bool isValidUrl(const std::string &url) {
    HttpUri uri;
    try {
      uri.parse(url);
      return true;
    } catch (const std::runtime_error &err) {
      return false;
    }
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
      const SectorFileType &file_type,
      bool primary) {
    std::unique_lock lock(mutex_);

    for (const auto &type : primitives::sector_file::kSectorFileTypes) {
      if ((file_type & type) == 0) {
        continue;
      }

      Decl index{
          .sector_id = sector,
          .type = type,
      };

      auto sector_iter = sectors_.find(index);
      if (sector_iter == sectors_.end()) {
        sectors_[index] = {};
        sector_iter = sectors_.find(index);
      }

      bool is_duplicate = false;
      for (auto &sid : sector_iter->second) {
        if (storage_id == sid.id) {
          if (!sid.is_primary && primary) {
            sid.is_primary = true;
          } else {
            logger_->warn(
                "sector {} redeclared in {}", sectorName(sector), storage_id);
          }
          is_duplicate = true;
          break;
        }
      }

      if (is_duplicate) {
        continue;
      }

      sector_iter->second.push_back(DeclMeta{
          .id = storage_id,
          .is_primary = primary,
      });
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

      Decl index{
          .sector_id = sector,
          .type = type,
      };

      auto sector_iter = sectors_.find(index);
      if (sector_iter == sectors_.end()) {
        return outcome::success();
      }
      std::vector<DeclMeta> sectors;
      for (const auto &sid : sector_iter->second) {
        if (storage_id == sid.id) {
          continue;
        }
        sectors.push_back(sid);
      }
      if (sectors.empty()) {
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
      boost::optional<SectorSize> fetch_sector_size) {
    std::shared_lock lock(mutex_);
    struct StorageMeta {
      uint64_t storage_count;
      bool is_primary;
    };
    std::unordered_map<StorageID, StorageMeta> storages;

    for (const auto &type : primitives::sector_file::kSectorFileTypes) {
      if ((file_type & type) == 0) {
        continue;
      }

      Decl index{
          .sector_id = sector,
          .type = type,
      };
      auto sector_iter = sectors_.find(index);
      if (sector_iter == sectors_.end()) {
        continue;
      }
      for (const auto &storage : sector_iter->second) {
        ++(storages[storage.id].storage_count);
        storages[storage.id].is_primary =
            storages[storage.id].is_primary || storage.is_primary;
      }
    }

    std::vector<StorageInfo> result;
    for (const auto &[id, meta] : storages) {
      if (stores_.find(id) == stores_.end()) {
        // TODO (ortyomka): logger
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

      store.weight = store.weight * meta.storage_count;
      store.is_primary = storages[id].is_primary;
      result.push_back(store);
    }

    if (fetch_sector_size.has_value()) {
      OUTCOME_TRY(required_space,
                  primitives::sector_file::sealSpaceUse(
                      file_type, fetch_sector_size.get()));
      for (const auto &[id, storage_info] : stores_) {
        if (!storage_info.info.can_seal) {
          continue;
        }

        if (required_space
            > static_cast<uint64_t>(storage_info.fs_stat.available)) {
          logger_->debug(
              "not selecting on {}, out of space (available: {}, need: {})",
              storage_info.info.id,
              storage_info.fs_stat.available,
              required_space);
          continue;
        }

        if (duration_cast<std::chrono::seconds>(
                high_resolution_clock::now().time_since_epoch()
                - storage_info.last_heartbeat.time_since_epoch())
            > kSkippedHeartbeatThreshold) {
          logger_->debug(
              "not selecting on {}, didn't receive heartbeats for {}",
              storage_info.info.id,
              duration_cast<std::chrono::seconds>(
                  high_resolution_clock::now().time_since_epoch()
                  - storage_info.last_heartbeat.time_since_epoch())
                  .count());
        }

        if (storage_info.error.has_value()) {
          logger_->debug("not selecting on {}, heartbeat_interval_ error: {}",
                         storage_info.info.id,
                         storage_info.error.get());
          continue;
        }

        if (storages.find(id) != storages.end()) {
          continue;
        }
        auto store = storage_info.info;

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
        store.is_primary = false;
        result.push_back(store);
      }
    }

    return result;
  }

  outcome::result<std::vector<StorageInfo>> SectorIndexImpl::storageBestAlloc(
      const fc::primitives::sector_file::SectorFileType &allocate,
      SectorSize sector_size,
      bool sealing_mode) {
    std::shared_lock lock(mutex_);

    OUTCOME_TRY(
        req_space,
        fc::primitives::sector_file::sealSpaceUse(allocate, sector_size));

    std::vector<StorageEntry> candidates;

    for (const auto &[id, storage] : stores_) {
      if (sealing_mode && !storage.info.can_seal) {
        continue;
      }
      if (!sealing_mode && !storage.info.can_store) {
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
                return BigInt{lhs.fs_stat.available} * lhs.info.weight
                       < BigInt{rhs.fs_stat.available} * rhs.info.weight;
              });

    std::vector<StorageInfo> result;

    for (const auto &candidate : candidates) {
      result.emplace_back(candidate.info);
    }

    return result;
  }

  outcome::result<std::unique_ptr<WLock>> SectorIndexImpl::storageLock(
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

  std::unique_ptr<WLock> SectorIndexImpl::storageTryLock(const SectorId &sector,
                                                         SectorFileType read,
                                                         SectorFileType write) {
    auto lock = std::make_unique<IndexLock::Lock>(sector, read, write);

    auto is_locked = index_lock_->lock(*lock, false);

    if (is_locked) {
      return std::move(lock);
    }

    return nullptr;
  }

  SectorIndexImpl::SectorIndexImpl() {
    index_lock_ = std::make_shared<IndexLock>();
    logger_ = common::createLogger("sector index");
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
