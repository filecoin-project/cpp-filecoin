/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/stores/impl/remote_store.hpp"

#include <curl/curl.h>
#include <rapidjson/document.h>
#include <boost/filesystem.hpp>
#include <utility>
#include "api/rpc/json.hpp"
#include "common/tarutil.hpp"
#include "common/uri_parser/uri_parser.hpp"
#include "sector_storage/stores/store_error.hpp"

namespace fs = boost::filesystem;

namespace {
  std::size_t callbackString(const char *in,
                             std::size_t size,
                             std::size_t num,
                             std::string *out) {
    const std::size_t totalBytes(size * num);
    out->append(in, totalBytes);
    return totalBytes;
  }

  std::size_t callbackFile(const char *ptr,
                           std::size_t size,
                           std::size_t nmemb,
                           std::ofstream *stream) {
    const std::size_t totalBytes(size * nmemb);
    stream->write(ptr, size);
    return totalBytes;
  }
}  // namespace

namespace fc::sector_storage::stores {

  RemoteStoreImpl::RemoteStoreImpl(
      std::shared_ptr<LocalStore> local,
      std::unordered_map<HeaderName, HeaderValue> auth_headers)
      : local_(std::move(local)),
        sector_index_(local_->getSectorIndex()),
        auth_headers_(std::move(auth_headers)),
        logger_{common::createLogger("remote store")} {}

  outcome::result<AcquireSectorResponse> RemoteStoreImpl::acquireSector(
      SectorId sector,
      RegisteredProof seal_proof_type,
      SectorFileType existing,
      SectorFileType allocate,
      bool can_seal) {
    if ((existing & allocate) != 0) {
      return StoreErrors::kFindAndAllocate;
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

    auto _ = gsl::finally([&]() {
      const std::lock_guard<std::mutex> lock(mutex_);
      processing_.erase(sector);
      unlock_ = true;
      cv_.notify_all();
    });

    auto maybe_response = local_->acquireSector(
        sector, seal_proof_type, existing, allocate, can_seal);

    if (maybe_response.has_error()) {
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
        return maybe_remote_response.error();
      }

      response.paths.setPathByType(type, maybe_remote_response.value().path);
      response.storages.setPathByType(type,
                                      maybe_remote_response.value().storage_id);

      auto maybe_err = sector_index_->storageDeclareSector(
          maybe_remote_response.value().storage_id, sector, type);
      if (maybe_err.has_error()) {
        logger_->warn("acquireSector: failed to declare sector {} - {}",
                      primitives::sector_file::sectorName(sector),
                      maybe_err.error().message());
        continue;
      }
    }

    return std::move(response);
  }

  outcome::result<void> RemoteStoreImpl::remove(SectorId sector,
                                                SectorFileType type) {
    OUTCOME_TRY(local_->remove(sector, type));
    OUTCOME_TRY(infos, sector_index_->storageFindSector(sector, type, false));

    for (const auto &info : infos) {
      for (const auto &url : info.urls) {
        auto maybe_error = deleteFromRemote(url);
        if (maybe_error.has_error()) {
          logger_->warn("remove: failed to remove from {} - {}",
                        url,
                        maybe_error.error().message());
          continue;
        }
        break;
      }
    }

    return outcome::success();
  }

  outcome::result<void> RemoteStoreImpl::moveStorage(
      SectorId sector, RegisteredProof seal_proof_type, SectorFileType types) {
    OUTCOME_TRY(acquireSector(
        sector, seal_proof_type, types, SectorFileType::FTNone, false));
    return local_->moveStorage(sector, seal_proof_type, types);
  }

  outcome::result<FsStat> RemoteStoreImpl::getFsStat(StorageID id) {
    auto maybe_stat = local_->getFsStat(id);
    if (maybe_stat != outcome::failure(StoreErrors::kNotFoundStorage)) {
      return maybe_stat;
    }

    OUTCOME_TRY(info, sector_index_->getStorageInfo(id));

    if (info.urls.empty()) {
      logger_->error("Remote Store: no known URLs for remote storage {}", id);
      return StoreErrors::kNoRemoteStorageUrls;
    }

    fc::common::HttpUri parser;
    try {
      parser.parse(info.urls[0]);
    } catch (const std::runtime_error &e) {
      return StoreErrors::kInvalidUrl;
    }

    parser.setPath((fs::path(parser.path()) / "stat" / id).string());

    CURL *curl = curl_easy_init();

    if (!curl) {
      return StoreErrors::kUnableCreateRequest;
    }
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

    // Follow HTTP redirects if necessary
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    curl_easy_setopt(curl, CURLOPT_URL, parser.str().c_str());

    struct curl_slist *headers = nullptr;

    for (const auto &header : auth_headers_) {
      headers = curl_slist_append(
          headers, (header.first + ": " + header.second).c_str());
    }

    std::string body;

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callbackString);

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);

    if (headers) {
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    long status_code;
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
    curl_easy_cleanup(curl);
    if (headers) {
      curl_slist_free_all(headers);
    }

    switch (status_code) {
      case 404:
        return StoreErrors::kNotFoundPath;
      case 500:
        logger_->error("getFsStat: 500 error received - {}", body);
        return StoreErrors::kInternalServerError;
      default:
        break;
    }

    rapidjson::Document j_file;
    j_file.Parse(body.data(), body.size());
    body.clear();
    if (j_file.HasParseError()) {
      return StoreErrors::kInvalidFsStatResponse;
    }

    return api::decode<FsStat>(j_file);
  }

  outcome::result<RemoteStoreImpl::RemoveAcquireSectorResponse>
  RemoteStoreImpl::acquireFromRemote(SectorId sector,
                                     RegisteredProof seal_proof_type,
                                     SectorFileType file_type,
                                     bool can_seal) {
    OUTCOME_TRY(infos,
                sector_index_->storageFindSector(sector, file_type, false));

    if (infos.empty()) {
      return StoreErrors::kNotFoundSector;
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
          logger_->warn("acquireFromRemote: failed to acqiure from {} - {}",
                        url,
                        maybe_error.error().message());
          continue;
        }

        res.url = url;

        return res;
      }
    }

    return StoreErrors::kUnableRemoteAcquireSector;
  }

  outcome::result<void> RemoteStoreImpl::fetch(const std::string &url,
                                               const std::string &output_path) {
    logger_->info("fetch: {} -> {}", url, output_path);

    CURL *curl = curl_easy_init();

    if (!curl) {
      return StoreErrors::kUnableCreateRequest;
    }
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

    // Follow HTTP redirects if necessary
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    fs::path temp_file_path =
        fs::temp_directory_path() / (fs::unique_path().string() + ".tar");

    std::ofstream temp_file(temp_file_path.string(),
                            std::ios_base::out | std::ios_base::binary);

    if (!temp_file.good()) {
      curl_easy_cleanup(curl);
      return StoreErrors::kCannotOpenTempFile;
    }

    struct curl_slist *headers = nullptr;

    for (const auto &header : auth_headers_) {
      headers = curl_slist_append(
          headers, (header.first + ": " + header.second).c_str());
    }

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callbackFile);

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &temp_file);

    if (headers) {
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    long status_code;
    std::string content_type;
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
    curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &content_type);
    curl_easy_cleanup(curl);
    if (headers) {
      curl_slist_free_all(headers);
    }

    temp_file.close();

    if (status_code != 200) {
      logger_->error("non-200 code - {}", status_code);
      return StoreErrors::kNotOkStatusCode;
    }

    boost::system::error_code ec;
    fs::remove_all(output_path, ec);

    if (ec.failed()) {
      logger_->error("Cannot remove output path: {}", ec.message());
      return StoreErrors::kCannotRemoveOutputPath;
    }
    ec.clear();

    if (content_type == "application/x-tar") {
      auto result =
          fc::common::extractTar(temp_file_path.string(), output_path);
      fs::remove_all(temp_file_path, ec);
      if (ec.failed()) {
        logger_->warn("fetch: failed to remove archive - {}", ec.message());
      }
      return result;
    }

    if (content_type == "application/octet-stream") {
      fs::rename(temp_file_path, output_path, ec);
      if (ec.failed()) {
        logger_->error("Cannot move file: {}", ec.message());
        return StoreErrors::kCannotMoveFile;
      }
      return outcome::success();
    }

    return StoreErrors::kUnknownContentType;
  }

  outcome::result<void> RemoteStoreImpl::deleteFromRemote(
      const std::string &url) {
    logger_->info("delete from remote: {}", url);

    CURL *curl = curl_easy_init();

    if (!curl) {
      return StoreErrors::kUnableCreateRequest;
    }
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

    // Follow HTTP redirects if necessary
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");

    struct curl_slist *headers = nullptr;

    for (const auto &header : auth_headers_) {
      headers = curl_slist_append(
          headers, (header.first + ": " + header.second).c_str());
    }

    long status_code;
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
    curl_easy_cleanup(curl);
    if (headers) {
      curl_slist_free_all(headers);
    }

    if (status_code != 200) {
      logger_->error("non-200 code - {}", status_code);
      return StoreErrors::kNotOkStatusCode;
    }

    return outcome::success();
  }

  std::shared_ptr<SectorIndex> RemoteStoreImpl::getSectorIndex() const {
    return sector_index_;
  }

  std::shared_ptr<LocalStore> RemoteStoreImpl::getLocalStore() const {
    return local_;
  }

}  // namespace fc::sector_storage::stores
