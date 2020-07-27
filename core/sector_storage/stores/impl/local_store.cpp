/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/stores/impl/local_store.hpp"

#include <rapidjson/document.h>
#include <boost/filesystem.hpp>
#include <fstream>
#include <regex>
#include <utility>
#include "api/rpc/json.hpp"
#include "primitives/sector_file/sector_file.hpp"
#include "sector_storage/stores/store_error.hpp"

using fc::primitives::LocalStorageMeta;
using fc::primitives::sector_file::kSectorFileTypes;

namespace fc::sector_storage::stores {

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
      bool can_seal) {
    if ((existing & allocate) != 0) {
      return StoreErrors::kFindAndAllocate;
    }

    std::shared_lock lock(mutex_);

    AcquireSectorResponse result{};

    for (const auto &type : primitives::sector_file::kSectorFileTypes) {
      if ((type & existing) == 0) {
        continue;
      }

      auto storages_info_opt = index_->storageFindSector(sector, type, false);

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

        if (store_path_iter->second.empty()) {
          continue;
        }

        boost::filesystem::path spath(store_path_iter->second);
        spath /= toString(type);
        spath /= primitives::sector_file::sectorName(sector);

        result.paths.setPathByType(type, spath.string());
        result.storages.setPathByType(type, info.id);

        existing = static_cast<SectorFileType>(existing ^ type);
        break;
      }

      OUTCOME_TRY(path, result.paths.getPathByType(type));

      if (path.empty()) {
        return StoreErrors::kNotFoundRequestedSectorType;
      }
    }

    for (const auto &type : primitives::sector_file::kSectorFileTypes) {
      if ((type & allocate) == 0) {
        continue;
      }

      OUTCOME_TRY(sectors_info,
                  index_->storageBestAlloc(type, seal_proof_type, can_seal));

      std::string best_path;
      StorageID best_storage;

      for (const auto &info : sectors_info) {
        auto path_iter = paths_.find(info.id);
        if (path_iter == paths_.end()) {
          continue;
        }

        if (path_iter->second.empty()) {
          continue;
        }

        boost::filesystem::path spath(path_iter->second);
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

    OUTCOME_TRY(storages_info, index_->storageFindSector(sector, type, false));

    if (storages_info.empty()) {
      return StoreErrors::kNotFoundSector;
    }

    for (const auto &info : storages_info) {
      auto path_iter = paths_.find(info.id);
      if (path_iter == paths_.end()) {
        continue;
      }

      if (path_iter->second.empty()) {
        continue;
      }

      OUTCOME_TRY(index_->storageDropSector(info.id, sector, type));

      boost::filesystem::path sector_path(path_iter->second);
      sector_path /= toString(type);
      sector_path /= primitives::sector_file::sectorName(sector);

      logger_->info("Remove " + sector_path.string());

      boost::system::error_code ec;
      boost::filesystem::remove_all(sector_path, ec);
      if (ec.failed()) {
        logger_->error(ec.message());
        return StoreErrors::kCannotRemoveSector;
      }
    }

    return outcome::success();
  }

  outcome::result<void> LocalStoreImpl::moveStorage(
      SectorId sector, RegisteredProof seal_proof_type, SectorFileType types) {
    OUTCOME_TRY(
        dest,
        acquireSector(
            sector, seal_proof_type, SectorFileType::FTNone, types, false));

    OUTCOME_TRY(
        src,
        acquireSector(
            sector, seal_proof_type, types, SectorFileType::FTNone, false));

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

      OUTCOME_TRY(index_->storageDeclareSector(dest_storage_id, sector, type));
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

    return storage_->getStat(path_iter->second);
  }

  outcome::result<void> LocalStoreImpl::openPath(const std::string &path) {
    std::unique_lock lock(mutex_);
    auto root = boost::filesystem::path(path);
    std::ifstream file{(root / kMetaFileName).string(),
                       std::ios::binary | std::ios::ate};
    if (!file.good()) {
      return StoreErrors::kInvalidStorageConfig;
    }
    fc::common::Buffer buffer;
    buffer.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(fc::common::span::string(buffer).data(), buffer.size());

    rapidjson::Document j_file;
    j_file.Parse(fc::common::span::cstring(buffer).data(), buffer.size());
    buffer.clear();
    if (j_file.HasParseError()) {
      return StoreErrors::kInvalidStorageConfig;
    }
    OUTCOME_TRY(meta, api::decode<LocalStorageMeta>(j_file));

    auto path_iter = paths_.find(meta.id);
    if (path_iter != paths_.end()) {
      return StoreErrors::kDuplicateStorage;
    }

    OUTCOME_TRY(stat, storage_->getStat(path));

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

        OUTCOME_TRY(index_->storageDeclareSector(meta.id, sector, type));

        ++dir_iter;
      }
    }

    OUTCOME_TRY(storage_->setStorage([path](stores::StorageConfig &config) {
      config.storage_paths.push_back(std::move(path));
    }));
    paths_[meta.id] = path;

    return outcome::success();
  }

  outcome::result<std::unique_ptr<LocalStore>> LocalStoreImpl::newLocalStore(
      const std::shared_ptr<LocalStorage> &storage,
      const std::shared_ptr<SectorIndex> &index,
      gsl::span<const std::string> urls) {
    struct make_unique_enabler : public LocalStoreImpl {
      make_unique_enabler(const std::shared_ptr<LocalStorage> &storage,
                          const std::shared_ptr<SectorIndex> &index,
                          gsl::span<const std::string> urls)
          : LocalStoreImpl{storage, index, urls} {};
    };

    std::unique_ptr<LocalStoreImpl> local =
        std::make_unique<make_unique_enabler>(storage, index, urls);

    if (local->logger_ == nullptr) {
      return StoreErrors::kCannotInitLogger;
    }

    OUTCOME_TRY(config, local->storage_->getStorage());

    for (const auto &path : config.storage_paths) {
      OUTCOME_TRY(local->openPath(path));
    }

    return std::move(local);
  }

  outcome::result<std::vector<primitives::StoragePath>>
  LocalStoreImpl::getAccessiblePaths() {
    std::shared_lock lock(mutex_);

    std::vector<primitives::StoragePath> res;
    for (const auto &[id, path] : paths_) {
      if (path.empty()) {
        continue;
      }

      OUTCOME_TRY(info, index_->getStorageInfo(id));

      res.push_back(primitives::StoragePath{
          .id = id,
          .weight = info.weight,
          .local_path = path,
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

}  // namespace fc::sector_storage::stores
