/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/stores/impl/local_store.hpp"

#include <time.h>
#include <boost/filesystem.hpp>
#include <chrono>
#include <libp2p/protocol/common/asio/asio_scheduler.hpp>
#include <map>
#include <random>
#include <regex>
#include <utility>

#include "api/rpc/json.hpp"
#include "codec/json/json.hpp"
#include "common/file.hpp"
#include "primitives/sector_file/sector_file.hpp"
#include "sector_storage/stores/impl/util.hpp"
#include "sector_storage/stores/storage_error.hpp"
#include "sector_storage/stores/store_error.hpp"

using fc::primitives::LocalStorageMeta;
using fc::primitives::sector_file::kSectorFileTypes;
using std::chrono::duration_cast;

namespace fc::sector_storage::stores {
  namespace fs = boost::filesystem;

  outcome::result<SectorId> parseSectorId(const std::string &filename) {
    std::regex regex(R"(s-t0([0-9]+)-([0-9]+))");
    std::smatch sm;
    if (std::regex_match(filename, sm, regex)) {
      try {
        auto miner = boost::lexical_cast<primitives::ActorId>(sm[1]);
        auto sector = boost::lexical_cast<primitives::SectorNumber>(sm[2]);
        return SectorId{
            .miner = miner,
            .sector = sector,
        };
      } catch (const boost::bad_lexical_cast &) {
        return StoreErrors::kInvalidSectorName;
      }
    }
    return StoreErrors::kInvalidSectorName;
  }

  LocalStoreImpl::LocalStoreImpl(std::shared_ptr<LocalStorage> storage,
                                 std::shared_ptr<SectorIndex> index,
                                 gsl::span<const std::string> urls)
      : storage_(std::move(storage)),
        index_(std::move(index)),
        urls_(urls.begin(), urls.end()) {
    logger_ = common::createLogger("Local Store");
  }

  outcome::result<AcquireSectorResponse> LocalStoreImpl::acquireSector(
      SectorId sector,
      RegisteredProof seal_proof_type,
      SectorFileType existing,
      SectorFileType allocate,
      PathType path_type,
      AcquireMode mode) {
    if ((existing & allocate) != 0) {
      return StoreErrors::kFindAndAllocate;
    }

    std::shared_lock lock(mutex_);

    AcquireSectorResponse result{};

    for (const auto &type : primitives::sector_file::kSectorFileTypes) {
      if ((type & existing) == 0) {
        continue;
      }

      auto storages_info_opt =
          index_->storageFindSector(sector, type, boost::none);

      if (storages_info_opt.has_error()) {
        logger_->warn("Finding existing sector: "
                      + storages_info_opt.error().message());
        continue;
      }

      for (const auto &info : storages_info_opt.value()) {
        auto store_path_iter = paths_.find(info.id);
        if (store_path_iter == paths_.end()) {
          continue;
        }

        if (store_path_iter->second->local_path.empty()) {
          continue;
        }

        boost::filesystem::path spath(store_path_iter->second->local_path);
        spath /= toString(type);
        spath /= primitives::sector_file::sectorName(sector);

        result.paths.setPathByType(type, spath.string());
        result.storages.setPathByType(type, info.id);

        existing = static_cast<SectorFileType>(existing ^ type);
        break;
      }
    }

    for (const auto &type : primitives::sector_file::kSectorFileTypes) {
      if ((type & allocate) == 0) {
        continue;
      }

      OUTCOME_TRY(sectors_info,
                  index_->storageBestAlloc(
                      type, seal_proof_type, path_type == PathType::kSealing));

      std::string best_path;
      StorageID best_storage;

      for (const auto &info : sectors_info) {
        auto path_iter = paths_.find(info.id);
        if (path_iter == paths_.end()) {
          continue;
        }

        if (path_iter->second->local_path.empty()) {
          continue;
        }

        if (path_type == PathType::kSealing && !info.can_seal) {
          continue;
        }

        if (path_type == PathType::kStorage && !info.can_store) {
          continue;
        }

        boost::filesystem::path spath(path_iter->second->local_path);
        spath /= toString(type);
        spath /= primitives::sector_file::sectorName(sector);

        best_path = spath.string();
        best_storage = info.id;
        break;
      }

      if (best_path.empty()) {
        return StoreErrors::kNotFoundPath;
      }

      result.paths.setPathByType(type, best_path);
      result.storages.setPathByType(type, best_storage);
      allocate = static_cast<SectorFileType>(allocate ^ type);
    }

    return result;
  }

  outcome::result<void> LocalStoreImpl::remove(SectorId sector,
                                               SectorFileType type) {
    if (type == SectorFileType::FTNone || ((type & (type - 1)) != 0)) {
      return StoreErrors::kRemoveSeveralFileTypes;
    }

    OUTCOME_TRY(storages_info,
                index_->storageFindSector(sector, type, boost::none));

    for (const auto &info : storages_info) {
      OUTCOME_TRY(removeSector(sector, type, info.id));
    }

    return outcome::success();
  }

  outcome::result<void> LocalStoreImpl::removeCopies(SectorId sector,
                                                     SectorFileType type) {
    if (type == SectorFileType::FTNone || ((type & (type - 1)) != 0)) {
      return StoreErrors::kRemoveSeveralFileTypes;
    }

    OUTCOME_TRY(infos, index_->storageFindSector(sector, type, boost::none));

    bool has_primary = false;
    for (const auto &info : infos) {
      if (info.is_primary) {
        has_primary = true;
        break;
      }
    }

    if (!has_primary) {
      logger_->warn(
          "RemoveCopies: no primary copies of sector {} ({}), not removing "
          "anything",
          primitives::sector_file::sectorName(sector),
          type);
      return outcome::success();
    }

    for (const auto &info : infos) {
      if (info.is_primary) {
        continue;
      }

      OUTCOME_TRY(removeSector(sector, type, info.id));
    }

    return outcome::success();
  }

  outcome::result<void> LocalStoreImpl::removeSector(SectorId sector,
                                                     SectorFileType type,
                                                     const StorageID &storage) {
    auto path_iter = paths_.find(storage);
    if (path_iter == paths_.end()) {
      return outcome::success();
    }

    if (path_iter->second->local_path.empty()) {
      return outcome::success();
    }

    OUTCOME_TRY(index_->storageDropSector(storage, sector, type));

    boost::filesystem::path sector_path(path_iter->second->local_path);
    sector_path /= toString(type);
    sector_path /= primitives::sector_file::sectorName(sector);

    logger_->info("Remove " + sector_path.string());

    boost::system::error_code ec;
    boost::filesystem::remove_all(sector_path, ec);
    if (ec.failed()) {
      logger_->error(ec.message());
    }
    return outcome::success();
  }

  outcome::result<void> LocalStoreImpl::moveStorage(
      SectorId sector, RegisteredProof seal_proof_type, SectorFileType types) {
    OUTCOME_TRY(dest,
                acquireSector(sector,
                              seal_proof_type,
                              SectorFileType::FTNone,
                              types,
                              PathType::kStorage,
                              AcquireMode::kMove));

    OUTCOME_TRY(src,
                acquireSector(sector,
                              seal_proof_type,
                              types,
                              SectorFileType::FTNone,
                              PathType::kStorage,
                              AcquireMode::kMove));

    for (const auto &type : kSectorFileTypes) {
      if ((types & type) == 0) {
        continue;
      }

      OUTCOME_TRY(source_storage_id, src.storages.getPathByType(type));
      OUTCOME_TRY(sst, index_->getStorageInfo(source_storage_id));

      OUTCOME_TRY(dest_storage_id, dest.storages.getPathByType(type));
      OUTCOME_TRY(dst, index_->getStorageInfo(dest_storage_id));

      if (sst.id == dst.id) {
        continue;
      }

      if (sst.can_store) {
        continue;
      }
      OUTCOME_TRY(index_->storageDropSector(source_storage_id, sector, type));

      OUTCOME_TRY(source_path, src.paths.getPathByType(type));
      OUTCOME_TRY(dest_path, dest.paths.getPathByType(type));

      boost::system::error_code ec;
      boost::filesystem::rename(source_path, dest_path, ec);
      if (ec.failed()) {
        return StoreErrors::kCannotMoveSector;
      }

      OUTCOME_TRY(
          index_->storageDeclareSector(dest_storage_id, sector, type, true));
    }

    return outcome::success();
  }

  outcome::result<FsStat> LocalStoreImpl::getFsStat(
      fc::primitives::StorageID id) {
    std::shared_lock lock(mutex_);

    auto path_iter = paths_.find(id);
    if (path_iter == paths_.end()) {
      return StoreErrors::kNotFoundStorage;
    }

    return storage_->getStat(path_iter->second->local_path);
  }

  outcome::result<void> LocalStoreImpl::openPath(const std::string &path) {
    std::unique_lock lock(mutex_);
    auto root = boost::filesystem::path(path);
    OUTCOME_TRY(text, common::readFile((root / kMetaFileName).string()));
    OUTCOME_TRY(j_file, codec::json::parse(text));
    OUTCOME_TRY(meta, api::decode<LocalStorageMeta>(j_file));

    auto path_iter = paths_.find(meta.id);
    if (path_iter != paths_.end()) {
      return StoreErrors::kDuplicateStorage;
    }

    std::shared_ptr<Path> out = Path::newPath(path);

    OUTCOME_TRY(stat, out->getStat(storage_));

    OUTCOME_TRY(index_->storageAttach(
        StorageInfo{
            .id = meta.id,
            .urls = urls_,
            .weight = meta.weight,
            .can_seal = meta.can_seal,
            .can_store = meta.can_store,
        },
        stat));

    for (const auto &type : kSectorFileTypes) {
      auto dir_path = root / toString(type);
      if (!boost::filesystem::exists(dir_path)) {
        if (!boost::filesystem::create_directories(dir_path)) {
          return StoreErrors::kCannotCreateDir;
        }
        continue;
      }

      boost::filesystem::directory_iterator dir_iter(dir_path), end;
      while (dir_iter != end) {
        OUTCOME_TRY(sector,
                    parseSectorId(dir_iter->path().filename().string()));

        OUTCOME_TRY(index_->storageDeclareSector(
            meta.id, sector, type, meta.can_store));

        ++dir_iter;
      }
    }

    OUTCOME_TRY(storage_->setStorage([path](stores::StorageConfig &config) {
      auto &xs{config.storage_paths};
      if (std::find_if(
              xs.begin(), xs.end(), [&](auto &x) { return x.path == path; })
          == xs.end()) {
        xs.push_back({std::move(path)});
      }
    }));
    paths_[meta.id] = out;

    return outcome::success();
  }

  outcome::result<std::shared_ptr<LocalStore>> LocalStoreImpl::newLocalStore(
      const std::shared_ptr<LocalStorage> &storage,
      const std::shared_ptr<SectorIndex> &index,
      gsl::span<const std::string> urls,
      std::shared_ptr<Scheduler> scheduler) {
    struct make_unique_enabler : public LocalStoreImpl {
      make_unique_enabler(const std::shared_ptr<LocalStorage> &storage,
                          const std::shared_ptr<SectorIndex> &index,
                          gsl::span<const std::string> urls)
          : LocalStoreImpl{storage, index, urls} {};
    };
    std::shared_ptr<LocalStoreImpl> local =
        std::make_unique<make_unique_enabler>(storage, index, urls);

    if (local->logger_ == nullptr) {
      return StoreErrors::kCannotInitLogger;
    }

    OUTCOME_TRY(config, local->storage_->getStorage());
    std::mt19937 rng(std::time(0));
    std::uniform_int_distribution<> gen(0, 1000);
    local->heartbeat_interval_ =
        duration_cast<std::chrono::milliseconds>(kHeartbeatInterval).count()
        + gen(rng);
    for (const auto &path : config.storage_paths) {
      OUTCOME_TRY(local->openPath(path.path));
    }
    local->handler_ =
        scheduler->schedule(local->heartbeat_interval_,
                            [self = std::weak_ptr<LocalStoreImpl>(local)]() {
                              auto shared_self = self.lock();
                              if (shared_self) {
                                shared_self->reportHealth();
                              }
                            });
    return std::move(local);
  }

  outcome::result<std::vector<primitives::StoragePath>>
  LocalStoreImpl::getAccessiblePaths() {
    std::shared_lock lock(mutex_);

    std::vector<primitives::StoragePath> res;
    for (const auto &[id, path] : paths_) {
      if (path->local_path.empty()) {
        continue;
      }

      OUTCOME_TRY(info, index_->getStorageInfo(id));

      res.push_back(primitives::StoragePath{
          .id = id,
          .weight = info.weight,
          .local_path = path->local_path,
          .can_seal = info.can_seal,
          .can_store = info.can_store,
      });
    }

    return std::move(res);
  }

  std::shared_ptr<SectorIndex> LocalStoreImpl::getSectorIndex() const {
    return index_;
  }

  std::shared_ptr<LocalStorage> LocalStoreImpl::getLocalStorage() const {
    return storage_;
  }

  outcome::result<std::function<void()>> LocalStoreImpl::reserve(
      RegisteredProof seal_proof_type,
      SectorFileType file_type,
      const SectorPaths &storages,
      PathType path_type) {
    OUTCOME_TRY(sector_size,
                primitives::sector::getSectorSize(seal_proof_type));

    std::unique_lock lock(mutex_);
    std::vector<std::pair<std::shared_ptr<Path>, int64_t>> items;
    auto release_function = [](auto &items) {
      for (auto &[path, overhead] : items) {
        path->reserved -= overhead;
      }
    };

    auto _ = gsl::finally([&]() {
      lock.unlock();
      if (!items.empty()) {
        release_function(items);
      }
    });

    for (const auto &type : kSectorFileTypes) {
      if ((type & file_type) == 0) {
        continue;
      }

      OUTCOME_TRY(id, storages.getPathByType(type));

      auto path_iter = paths_.find(id);
      if (path_iter == paths_.end()) {
        return StoreErrors::kNotFoundPath;
      }

      OUTCOME_TRY(stat, path_iter->second->getStat(storage_));

      uint64_t overhead =
          (path_type == PathType::kStorage
               ? primitives::sector_file::kOverheadFinalized.at(type)
               : primitives::sector_file::kOverheadSeal.at(type))
          * sector_size / primitives::sector_file::kOverheadDenominator;

      if (stat.available < overhead) {
        return StoreErrors::kCannotReserve;
      }

      path_iter->second->reserved += overhead;

      items.emplace_back(path_iter->second, overhead);
    }

    return [clear = std::move(release_function), items = std::move(items)]() {
      clear(items);
    };
  }

  void LocalStoreImpl::reportHealth() {
    std::map<StorageID, HealthReport> toReport;
    {
      std::shared_lock lock(mutex_);
      for (auto path : paths_) {
        auto stat = path.second->getStat(storage_);
        std::pair<StorageID, HealthReport> report;
        if (stat.has_error()) {
          report = std::make_pair(path.first,
                                  HealthReport{
                                      .stat = {},
                                      .error = stat.error().message(),
                                  });
        } else {
          report = std::make_pair(
              path.first,
              HealthReport{.stat = stat.value(), .error = boost::none});
        }
        toReport.insert(std::move(report));
      }
    }

    for (const auto &report : toReport) {
      auto maybe_error =
          index_->storageReportHealth(report.first, report.second);
      if (maybe_error.has_error()) {
        logger_->warn("Error reporting storage health for {}: {}",
                      report.first,
                      maybe_error.error().message());
      }
    }

    handler_.reschedule(heartbeat_interval_);
  }

  std::string LocalStoreImpl::Path::sectorPath(const SectorId &sid,
                                               SectorFileType type) const {
    return (fs::path(local_path) / toString(type)
            / primitives::sector_file::sectorName(sid))
        .string();
  }

  outcome::result<FsStat> LocalStoreImpl::Path::getStat(
      const std::shared_ptr<LocalStorage> &local_storage) const {
    OUTCOME_TRY(stat, local_storage->getStat(local_path));

    stat.reserved = reserved;

    for (const auto &[id, file_type] : reservations) {
      for (const auto &type : kSectorFileTypes) {
        if ((type & file_type) == 0) {
          continue;
        }

        auto sector_path = sectorPath(id, file_type);

        auto maybe_used = local_storage->getDiskUsage(sector_path);
        if (maybe_used.has_error()) {
          if (maybe_used != outcome::failure(StorageError::kFileNotExist)) {
            return maybe_used.error();
          }

          OUTCOME_TRY(path, tempFetchDest(sector_path, false));

          maybe_used = local_storage->getDiskUsage(path);
          if (maybe_used.has_error()) {
            return maybe_used.error();
          }
        }

        if (stat.reserved < maybe_used.value()) {
          stat.reserved = 0;
          break;
        } else {
          stat.reserved -= maybe_used.value();
        }
      }
      if (stat.reserved == 0) {
        break;
      }
    }

    if (stat.available < stat.reserved) {
      stat.available = 0;
    } else {
      stat.available -= stat.reserved;
    }

    return stat;
  }

  std::shared_ptr<LocalStoreImpl::Path> LocalStoreImpl::Path::newPath(
      std::string path) {
    struct make_unique_enabler : public LocalStoreImpl::Path {
      make_unique_enabler(std::string path) : Path{std::move(path)} {};
    };

    std::unique_ptr<Path> new_path =
        std::make_unique<make_unique_enabler>(std::move(path));

    return std::move(new_path);
  }

  LocalStoreImpl::Path::Path(std::string path) : local_path(std::move(path)) {}
}  // namespace fc::sector_storage::stores
