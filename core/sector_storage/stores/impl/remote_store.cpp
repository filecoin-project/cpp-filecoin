/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/stores/impl/remote_store.hpp"

#include <boost/filesystem.hpp>
#include "common/uri_parser/uri_parser.hpp"
#include "sector_storage/stores/store_error.hpp"

namespace fs = boost::filesystem;

namespace fc::sector_storage::stores {

  RemoteStore::RemoteStore(std::shared_ptr<LocalStore> local,
                           std::shared_ptr<SectorIndex> index)
      : local_(std::move(local)), index_(std::move(index)) {}

  outcome::result<AcquireSectorResponse> RemoteStore::acquireSector(
      SectorId sector,
      RegisteredProof seal_proof_type,
      SectorFileType existing,
      SectorFileType allocate,
      bool can_seal) {
    if ((existing | allocate) != (existing ^ allocate)) {
      return StoreErrors::FindAndAllocate;
    }

    // TODO: Lock Mutex with waiting

    OUTCOME_TRY(response,
                local_->acquireSector(
                    sector, seal_proof_type, existing, allocate, can_seal));

    for (const auto &type : primitives::sector_file::kSectorFileTypes) {
      if ((type & existing) == 0) {
        continue;
      }

      if (!response.paths.getPathByType(type).value().empty()) {
        continue;
      }

      OUTCOME_TRY(remote_response,
                  acquireFromRemote(sector, seal_proof_type, type, can_seal));

      response.paths.setPathByType(type, remote_response.path);
      response.stores.setPathByType(type, remote_response.storage_id);

      auto maybe_err = index_->storageDeclareSector(
          remote_response.storage_id, sector, type);
      if (maybe_err.has_error()) {
        // TODO: Log warn
        continue;
      }
    }

    return std::move(response);
  }

  outcome::result<void> RemoteStore::remove(SectorId sector,
                                            SectorFileType type) {
    OUTCOME_TRY(local_->remove(sector, type));
    OUTCOME_TRY(infos, index_->storageFindSector(sector, type, false));

    for (const auto &info : infos) {
      for (const auto &url : info.urls) {
        auto maybe_error = deleteFromRemote(url);
        if (maybe_error.has_error()) {
          // TODO: Log warning
          continue;
        }
        break;
      }
    }

    return outcome::success();
  }

  outcome::result<void> RemoteStore::moveStorage(
      SectorId sector, RegisteredProof seal_proof_type, SectorFileType types) {
    OUTCOME_TRY(acquireSector(
        sector, seal_proof_type, types, SectorFileType::FTNone, false));
    return local_->moveStorage(sector, seal_proof_type, types);
  }

  outcome::result<FsStat> RemoteStore::getFsStat(StorageID id) {
    auto maybe_stat = local_->getFsStat(id);
    if (maybe_stat.has_error()
        && maybe_stat != outcome::failure(StoreErrors::NotFoundStorage)) {
      return maybe_stat;
    }

    OUTCOME_TRY(info, index_->getStorageInfo(id));

    if (info.urls.empty()) {
      return outcome::success();  // TODO: ERROR
    }

    fc::common::HttpUri parser;
    try {
      parser.parse(info.urls[0]);
    } catch (const std::runtime_error &e) {
      return outcome::success();  // TODO: ERROR
    }

    parser.setPath((fs::path(parser.path()) / "stat" / id).string());

    // TODO: make request
    // TODO: add auth header
    // TODO: check response code
    // TODO: parse json answer

    return outcome::success();
  }

  outcome::result<RemoteStore::RemoveAcquireSectorResponse>
  RemoteStore::acquireFromRemote(SectorId sector,
                                 RegisteredProof seal_proof_type,
                                 SectorFileType file_type,
                                 bool can_seal) {
    return outcome::success();
  }

  outcome::result<void> RemoteStore::fetch(const std::string &url,
                                           const std::string &output_name) {
    return outcome::success();
  }

  outcome::result<void> RemoteStore::deleteFromRemote(const std::string &url) {
    return outcome::success();
  }

}  // namespace fc::sector_storage::stores
