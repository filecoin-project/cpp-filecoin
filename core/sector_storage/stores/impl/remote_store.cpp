/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/stores/impl/remote_store.hpp"

#include <curl/curl.h>
#include <boost/filesystem.hpp>
#include <utility>

#include "api/rpc/json.hpp"
#include "codec/json/json.hpp"
#include "common/tarutil.hpp"
#include "common/uri_parser/uri_parser.hpp"
#include "sector_storage/stores/impl/util.hpp"
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
      RegisteredSealProof seal_proof_type,
      SectorFileType existing,
      SectorFileType allocate,
      PathType path_type,
      AcquireMode mode) {
    if ((existing & allocate) != 0) {
      return StoreError::kFindAndAllocate;
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

    OUTCOME_TRY(
        response,
        local_->acquireSector(
            sector, seal_proof_type, existing, allocate, path_type, mode));

    int to_fetch = SectorFileType::FTNone;
    for (const auto &type : primitives::sector_file::kSectorFileTypes) {
      if ((type & existing) == 0) {
        continue;
      }

      if (response.paths.getPathByType(type).value().empty()) {
        to_fetch = to_fetch | type;
      }
    }

    OUTCOME_TRY(additional_paths,
                local_->acquireSector(sector,
                                      seal_proof_type,
                                      SectorFileType::FTNone,
                                      static_cast<SectorFileType>(to_fetch),
                                      path_type,
                                      mode));

    OUTCOME_TRY(release_storage,
                local_->reserve(seal_proof_type,
                                static_cast<SectorFileType>(to_fetch),
                                additional_paths.storages,
                                path_type));
    auto _1 = gsl::finally([&]() { release_storage(); });

    for (const auto &type : primitives::sector_file::kSectorFileTypes) {
      if ((type & existing) == 0) {
        continue;
      }

      if (!response.paths.getPathByType(type).value().empty()) {
        continue;
      }

      OUTCOME_TRY(dest, additional_paths.paths.getPathByType(type));
      OUTCOME_TRY(storage_id, additional_paths.storages.getPathByType(type));

      OUTCOME_TRY(url, acquireFromRemote(sector, type, dest));

      response.paths.setPathByType(type, dest);
      response.storages.setPathByType(type, storage_id);

      auto maybe_err = sector_index_->storageDeclareSector(
          storage_id, sector, type, mode == AcquireMode::kMove);
      if (maybe_err.has_error()) {
        logger_->warn("acquireSector: failed to declare sector {} - {}",
                      primitives::sector_file::sectorName(sector),
                      maybe_err.error().message());
        continue;
      }

      if (mode == AcquireMode::kMove) {
        auto maybe_error = deleteFromRemote(url);
        if (maybe_error.has_error()) {
          logger_->warn("deleting sector {} from {} (delete {}): {}",
                        primitives::sector_file::sectorName(sector),
                        storage_id,
                        url,
                        maybe_error.error().message());
        }
      }
    }

    return std::move(response);
  }

  outcome::result<void> RemoteStoreImpl::remove(SectorId sector,
                                                SectorFileType type) {
    OUTCOME_TRY(local_->remove(sector, type));
    OUTCOME_TRY(infos,
                sector_index_->storageFindSector(sector, type, boost::none));

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

  outcome::result<void> RemoteStoreImpl::removeCopies(SectorId sector,
                                                      SectorFileType type) {
    // TODO: maybe move to LocalStore only
    return local_->removeCopies(sector, type);
  }

  outcome::result<void> RemoteStoreImpl::moveStorage(
      SectorId sector,
      RegisteredSealProof seal_proof_type,
      SectorFileType types) {
    OUTCOME_TRY(acquireSector(sector,
                              seal_proof_type,
                              types,
                              SectorFileType::FTNone,
                              PathType::kStorage,
                              AcquireMode::kMove));
    return local_->moveStorage(sector, seal_proof_type, types);
  }

  outcome::result<FsStat> RemoteStoreImpl::getFsStat(StorageID id) {
    auto maybe_stat = local_->getFsStat(id);
    if (maybe_stat != outcome::failure(StoreError::kNotFoundStorage)) {
      return maybe_stat;
    }

    OUTCOME_TRY(info, sector_index_->getStorageInfo(id));

    if (info.urls.empty()) {
      logger_->error("Remote Store: no known URLs for remote storage {}", id);
      return StoreError::kNoRemoteStorageUrls;
    }

    fc::common::HttpUri parser;
    try {
      parser.parse(info.urls[0]);
    } catch (const std::runtime_error &e) {
      return StoreError::kInvalidUrl;
    }

    parser.setPath((fs::path(parser.path()) / "stat" / id).string());

    CURL *curl = curl_easy_init();

    if (!curl) {
      return StoreError::kUnableCreateRequest;
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
        return StoreError::kNotFoundPath;
      case 500:
        logger_->error("getFsStat: 500 error received - {}", body);
        return StoreError::kInternalServerError;
      default:
        break;
    }

    OUTCOME_TRY(j_file, codec::json::parse(body));
    return api::decode<FsStat>(j_file);
  }

  outcome::result<std::string> RemoteStoreImpl::acquireFromRemote(
      SectorId sector, SectorFileType file_type, const std::string &dest) {
    OUTCOME_TRY(
        infos,
        sector_index_->storageFindSector(sector, file_type, boost::none));

    if (infos.empty()) {
      return StoreError::kNotFoundSector;
    }

    std::sort(infos.begin(),
              infos.end(),
              [](const auto &lhs, const auto &rhs) -> bool {
                return lhs.weight < rhs.weight;
              });

    for (const auto &info : infos) {
      OUTCOME_TRY(temp_dest, tempFetchDest(dest, true, logger_));
      for (const auto &url : info.urls) {
        boost::system::error_code ec;
        fs::remove_all(temp_dest, ec);
        if (ec.failed()) {
          logger_->error("cannot remove temp path: {}", ec.message());
          return StoreError::kCannotRemovePath;
        }

        auto maybe_error = fetch(url, temp_dest);
        if (maybe_error.has_error()) {
          logger_->warn("acquireFromRemote: failed to acqiure from {} - {}",
                        url,
                        maybe_error.error().message());
          continue;
        }

        fs::rename(temp_dest, dest, ec);
        if (ec.failed()) {
          logger_->error("cannot move from temp to dest: {}", ec.message());
          return StoreError::kCannotMoveFile;
        }

        return url;
      }
    }

    return StoreError::kUnableRemoteAcquireSector;
  }

  outcome::result<void> RemoteStoreImpl::fetch(const std::string &url,
                                               const std::string &output_path) {
    logger_->info("fetch: {} -> {}", url, output_path);

    CURL *curl = curl_easy_init();

    if (!curl) {
      return StoreError::kUnableCreateRequest;
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
      return StoreError::kCannotOpenTempFile;
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
      return StoreError::kNotOkStatusCode;
    }

    boost::system::error_code ec;
    fs::remove_all(output_path, ec);

    if (ec.failed()) {
      logger_->error("Cannot remove output path: {}", ec.message());
      return StoreError::kCannotRemovePath;
    }
    ec.clear();

    if (content_type == "application/x-tar") {
      auto result = common::extractTar(temp_file_path, output_path);
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
        return StoreError::kCannotMoveFile;
      }
      return outcome::success();
    }

    return StoreError::kUnknownContentType;
  }

  outcome::result<void> RemoteStoreImpl::deleteFromRemote(
      const std::string &url) {
    logger_->info("delete from remote: {}", url);

    CURL *curl = curl_easy_init();

    if (!curl) {
      return StoreError::kUnableCreateRequest;
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
      return StoreError::kNotOkStatusCode;
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
