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
    return outcome::success();
  }

  outcome::result<void> RemoteStore::remove(SectorId sector,
                                            SectorFileType type) {
    return outcome::success();
  }

  outcome::result<void> RemoteStore::moveStorage(
      SectorId sector, RegisteredProof seal_proof_type, SectorFileType types) {
    return outcome::success();
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
