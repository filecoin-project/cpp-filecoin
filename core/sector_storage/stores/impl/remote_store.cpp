/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/stores/impl/remote_store.hpp"

#include <curl/curl.h>
#include <rapidjson/document.h>
#include <boost/filesystem.hpp>
#include "api/rpc/json.hpp"
#include "common/uri_parser/uri_parser.hpp"
#include "sector_storage/stores/store_error.hpp"

namespace fs = boost::filesystem;

namespace {
  std::size_t callback(const char *in,
                       std::size_t size,
                       std::size_t num,
                       std::string *out) {
    const std::size_t totalBytes(size * num);
    out->append(in, totalBytes);
    return totalBytes;
  }
}  // namespace

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

    while (true) {
      {
        const std::lock_guard<std::mutex> lock(mutex_);
        if (processing_.insert(sector).second) {
          unlock_ = false;
          break;
        }
      }

      std::unique_lock<std::mutex> lock(waiting_room_);

      cv_.wait(lock, [&] { return unlock_; });
    }

    auto clear = [&]() {
      const std::lock_guard<std::mutex> lock(mutex_);
      processing_.erase(sector);
      unlock_ = true;
      cv_.notify_all();
    };

    auto maybe_response = local_->acquireSector(
        sector, seal_proof_type, existing, allocate, can_seal);

    if (maybe_response.has_error()) {
      clear();
      return maybe_response.error();
    }

    auto response = maybe_response.value();

    for (const auto &type : primitives::sector_file::kSectorFileTypes) {
      if ((type & existing) == 0) {
        continue;
      }

      if (!response.paths.getPathByType(type).value().empty()) {
        continue;
      }

      auto maybe_remote_response =
          acquireFromRemote(sector, seal_proof_type, type, can_seal);

      if (maybe_remote_response.has_error()) {
        clear();
        return maybe_remote_response.error();
      }

      response.paths.setPathByType(type, maybe_remote_response.value().path);
      response.storages.setPathByType(type,
                                      maybe_remote_response.value().storage_id);

      auto maybe_err = index_->storageDeclareSector(
          maybe_remote_response.value().storage_id, sector, type);
      if (maybe_err.has_error()) {
        // TODO: Log warn
        continue;
      }
    }

    clear();
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

    CURL *curl = curl_easy_init();

    if (!curl) {
      return outcome::success();  // TODO: ERROR
    }

    curl_easy_setopt(curl, CURLOPT_URL, parser.str().c_str());

    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

    // Follow HTTP redirects if necessary
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // TODO: add auth header

    long httpCode;
    std::string httpData;

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &httpData);

    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curl);

    switch (httpCode) {
      case 404:
        return StoreErrors::NotFoundPath;
      case 500:
        // TODO: log error
        return outcome::success();  // TODO: error
      default:
        break;
    }

    rapidjson::Document j_file;
    j_file.Parse(httpData.data(), httpData.size());
    httpData.clear();
    if (j_file.HasParseError()) {
      return outcome::success();  // TODO: error
    }

    return api::decode<FsStat>(j_file);
  }

  outcome::result<RemoteStore::RemoveAcquireSectorResponse>
  RemoteStore::acquireFromRemote(SectorId sector,
                                 RegisteredProof seal_proof_type,
                                 SectorFileType file_type,
                                 bool can_seal) {
    OUTCOME_TRY(infos, index_->storageFindSector(sector, file_type, false));

    if (infos.empty()) {
      return StoreErrors::NotFoundSector;
    }

    std::sort(infos.begin(),
              infos.end(),
              [](const auto &lhs, const auto &rhs) -> bool {
                return lhs.weight < rhs.weight;
              });

    OUTCOME_TRY(response,
                local_->acquireSector(sector,
                                      seal_proof_type,
                                      SectorFileType::FTNone,
                                      file_type,
                                      can_seal));

    RemoveAcquireSectorResponse res{};
    OUTCOME_TRYA(res.path, response.paths.getPathByType(file_type));
    OUTCOME_TRYA(res.storage_id, response.storages.getPathByType(file_type));

    for (const auto &info : infos) {
      for (const auto &url : info.urls) {
        auto maybe_error = fetch(url, res.path);
        if (maybe_error.has_error()) {
          // TODO: log it
          continue;
        }

        res.url = url;

        return res;
      }
    }

    return outcome::success();
  }

  outcome::result<void> RemoteStore::fetch(const std::string &url,
                                           const std::string &output_name) {
    // TODO: Log it
    CURL *curl = curl_easy_init();

    if (!curl) {
      return outcome::success();  // TODO: ERROR
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

    // Follow HTTP redirects if necessary
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // TODO: add auth header

    // TODO: Add callback

    long httpCode;

    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    char *ct = nullptr;
    curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);
    curl_easy_cleanup(curl);

    if (httpCode != 200) {
      return outcome::success();  // TODO: ERROR
    }

    if (!ct) {
      return outcome::success();  // TODO: ERROR
    }

    boost::system::error_code ec;
    fs::remove_all(output_name, ec);

    if (ec.failed()) {
      return outcome::success();  // TODO: ERROR
    }

    std::string mediatype(ct);

    if (mediatype == "application/x-tar") {
      // TODO: Processing tar
      return outcome::success();
    }

    if (mediatype == "application/octet-stream") {
      // TODO: Write to file
      return outcome::success();
    }

    return outcome::success();  // TODO: ERROR
  }

  outcome::result<void> RemoteStore::deleteFromRemote(const std::string &url) {
    // TODO: Log it

    CURL *curl = curl_easy_init();

    if (!curl) {
      return outcome::success();  // TODO: ERROR
    }

    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

    // Follow HTTP redirects if necessary
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // TODO: add auth header

    long httpCode;

    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curl);

    if (httpCode != 200) {
      return outcome::success();  // TODO: ERROR
    }

    return outcome::success();
  }

}  // namespace fc::sector_storage::stores
