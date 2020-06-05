/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/stores/impl/local_store.hpp"

#include <rapidjson/document.h>
#include <boost/filesystem.hpp>
#include <fstream>
#include <utility>
#include "api/rpc/json.hpp"
#include "primitives/sector_file/sector_file.hpp"
#include "sector_storage/stores/store_error.hpp"

using fc::primitives::LocalStorageMeta;
using fc::primitives::sector_file::kSectorFileTypes;

namespace fc::sector_storage::stores {

  outcome::result<SectorId> parseSectorId(const std::string &filename) {
    return outcome::success();  // TODO: ERROR
  }

  LocalStore::LocalStore(std::shared_ptr<LocalStorage> storage,
                         std::shared_ptr<SectorIndex> index,
                         gsl::span<std::string> urls)
      : storage_(std::move(storage)),
        index_(std::move(index)),
        urls_(urls.begin(), urls.end()) {}

  outcome::result<AcquireSectorResponse> LocalStore::acquireSector(
      SectorId sector,
      RegisteredProof seal_proof_type,
      SectorFileType existing,
      SectorFileType allocate,
      bool can_seal) {
    if ((existing | allocate) != (existing ^ allocate)) {
      return StoreErrors::FindAndAllocate;
    }

    std::shared_lock lock(mutex_);

    return acquireSectorWithoutLock(
        sector, seal_proof_type, existing, allocate, can_seal);
  }

  outcome::result<void> LocalStore::remove(SectorId sector,
                                           SectorFileType type) {
    // TODO: Check that delete only one type

    OUTCOME_TRY(storages_info, index_->storageFindSector(sector, type, false));

    if (storages_info.empty()) {
      return outcome::success();  // TODO: ERROR
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

      // TODO: Log about removing

      boost::system::error_code ec;
      boost::filesystem::remove_all(sector_path, ec);
      if (ec.failed()) {
        return outcome::success();  // TODO: ERROR
      }
    }

    return outcome::success();
  }

  outcome::result<void> LocalStore::moveStorage(SectorId sector,
                                                RegisteredProof seal_proof_type,
                                                SectorFileType types) {
    std::unique_lock lock(mutex_);
    OUTCOME_TRY(
        dest,
        acquireSectorWithoutLock(
            sector, seal_proof_type, SectorFileType::FTNone, types, false));

    OUTCOME_TRY(
        src,
        acquireSectorWithoutLock(
            sector, seal_proof_type, types, SectorFileType::FTNone, false));

    for (const auto &type : kSectorFileTypes) {
      if ((types & type) == 0) {
        continue;
      }

      OUTCOME_TRY(source_storage_id, src.stores.getPathByType(type));
      OUTCOME_TRY(sst, index_->getStorageInfo(source_storage_id));

      OUTCOME_TRY(dest_storage_id, dest.stores.getPathByType(type));
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
        return outcome::success();  // TODO: ERROR
      }

      OUTCOME_TRY(index_->storageDeclareSector(dest_storage_id, sector, type));
    }

    return outcome::success();
  }

  outcome::result<FsStat> LocalStore::getFsStat(fc::primitives::StorageID id) {
    std::shared_lock lock(mutex_);

    auto path_iter = paths_.find(id);
    if (path_iter == paths_.end()) {
      return outcome::success();  // TODO: ERROR;
    }

    return storage_->getStat(path_iter->second);
  }

  outcome::result<void> LocalStore::openPath(const std::string &path) {
    std::unique_lock lock(mutex_);
    auto root = boost::filesystem::path(path);
    std::ifstream file{(root / kMetaFileName).string(),
                       std::ios::binary | std::ios::ate};
    if (!file.good()) {
      return outcome::success();  // TODO: ERROR
    }
    fc::common::Buffer buffer;
    buffer.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(fc::common::span::string(buffer).data(), buffer.size());

    rapidjson::Document j_file;
    j_file.Parse(fc::common::span::cstring(buffer).data(), buffer.size());
    buffer.clear();
    if (j_file.HasParseError()) {
      return outcome::success();  // TODO: ERROR
    }
    OUTCOME_TRY(meta, api::decode<LocalStorageMeta>(j_file));

    auto path_iter = paths_.find(meta.id);
    if (path_iter != paths_.end()) {
      return outcome::success();  // TODO: ERROR
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
          return outcome::success();  // TODO: ERROR
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

    paths_[meta.id] = path;

    return outcome::success();
  }

  outcome::result<std::shared_ptr<LocalStore>> LocalStore::newLocalStore(
      std::shared_ptr<LocalStorage> storage,
      std::shared_ptr<SectorIndex> index,
      gsl::span<std::string> urls) {
    std::shared_ptr<LocalStore> local(
        new LocalStore(std::move(storage), std::move(index), urls));

    OUTCOME_TRY(paths, local->storage_->getPaths());

    for (const auto &path : paths) {
      OUTCOME_TRY(local->openPath(path));
    }

    return local;
  }

  outcome::result<AcquireSectorResponse> LocalStore::acquireSectorWithoutLock(
      SectorId sector,
      RegisteredProof seal_proof_type,
      SectorFileType existing,
      SectorFileType allocate,
      bool can_seal) {
    AcquireSectorResponse result{};

    for (const auto &type : primitives::sector_file::kSectorFileTypes) {
      if ((type & existing) == 0) {
        continue;
      }

      auto sectors_info_opt = index_->storageFindSector(sector, type, false);

      if (sectors_info_opt.has_error()) {
        // TODO: logger warning
        continue;
      }

      for (const auto &info : sectors_info_opt.value()) {
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
        result.stores.setPathByType(type, info.id);

        existing = static_cast<SectorFileType>(existing ^ type);
        break;
      }
    }

    for (const auto &type : primitives::sector_file::kSectorFileTypes) {
      if ((type & allocate) == 0) {
        continue;
      }

      auto sectors_info_opt =
          index_->storageBestAlloc(type, seal_proof_type, can_seal);

      if (sectors_info_opt.has_error()) {
        // TODO: logger
        return sectors_info_opt.error();
      }

      std::string best_path;
      StorageID best_storage;

      for (const auto &info : sectors_info_opt.value()) {
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
        return StoreErrors::NotFoundPath;
      }

      result.paths.setPathByType(type, best_path);
      result.stores.setPathByType(type, best_storage);
      allocate = static_cast<SectorFileType>(allocate ^ type);
    }

    return result;
  }

}  // namespace fc::sector_storage::stores
