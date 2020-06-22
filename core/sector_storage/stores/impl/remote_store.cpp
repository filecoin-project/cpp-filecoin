/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/stores/impl/remote_store.hpp"

#include <rapidjson/document.h>
#include <boost/filesystem.hpp>
#include <utility>
#include "api/rpc/json.hpp"
#include "common/tar_util.hpp"
#include "common/uri_parser/uri_parser.hpp"
#include "sector_storage/stores/store_error.hpp"

namespace fs = boost::filesystem;
using fc::common::ReqMethod;

namespace {
  std::size_t callbackString(const char *in,
                             std::size_t size,
                             std::size_t num,
                             void *out) {
    const std::size_t totalBytes(size * num);
    (static_cast<std::string *>(out))->append(in, totalBytes);
    return totalBytes;
  }

  std::size_t callbackFile(const char *ptr,
                           std::size_t size,
                           std::size_t nmemb,
                           void *stream) {
    const std::size_t totalBytes(size * nmemb);
    static_cast<std::ofstream *>(stream)->write(ptr, size);
    return totalBytes;
  }
}  // namespace

namespace fc::sector_storage::stores {

  RemoteStore::RemoteStore(
      std::shared_ptr<LocalStore> local,
      std::shared_ptr<SectorIndex> index,
      std::unordered_map<HeaderName, HeaderValue> auth_headers,
      std::shared_ptr<RequestFactory> request_factory)
      : local_(std::move(local)),
        index_(std::move(index)),
        auth_headers_(std::move(auth_headers)),
        request_factory_(std::move(request_factory)),
        logger_{common::createLogger("remote store")} {}

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
        logger_->warn("acquireSector: failed to declare sector {} - {}",
                      primitives::sector_file::sectorName(sector),
                      maybe_err.error().message());
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

    OUTCOME_TRY(req, request_factory_->newRequest(parser.str()));

    req->setupHeaders(auth_headers_);

    std::string body;

    req->setupWriteFunction(callbackString);

    req->setupWriteOutput(&body);

    auto res = req->perform();

    switch (res.status_code) {
      case 404:
        return StoreErrors::NotFoundPath;
      case 500:
        logger_->error("getFsStat: 500 error received - {}", body);
        return outcome::success();  // TODO: error
      default:
        break;
    }

    rapidjson::Document j_file;
    j_file.Parse(body.data(), body.size());
    body.clear();
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
          logger_->warn("acquireFromRemote: failed to acqiure from {} - {}",
                        url,
                        maybe_error.error().message());
          continue;
        }

        res.url = url;

        return res;
      }
    }

    return outcome::success();  // TODO: ERROR
  }

  outcome::result<void> RemoteStore::fetch(const std::string &url,
                                           const std::string &output_path) {
    logger_->info("fetch: {} -> {}", url, output_path);
    OUTCOME_TRY(req, request_factory_->newRequest(url));

    req->setupHeaders(auth_headers_);

    fs::path temp_file_path =
        fs::temp_directory_path() / (fs::unique_path().string() + ".tar");

    std::ofstream temp_file(temp_file_path.string(),
                            std::ios_base::out | std::ios_base::binary);

    if (!temp_file.good()) {
      return outcome::success();  // TODO: ERROR
    }

    req->setupWriteFunction(callbackFile);

    req->setupWriteOutput(&temp_file);

    auto res = req->perform();

    temp_file.close();
    if (res.status_code != 200) {
      return outcome::success();  // TODO: ERROR
    }

    boost::system::error_code ec;
    fs::remove_all(output_path, ec);

    if (ec.failed()) {
      return outcome::success();  // TODO: ERROR
    }
    ec.clear();

    if (res.content_type == "application/x-tar") {
      auto result =
          fc::common::extractTar(temp_file_path.string(), output_path);
      fs::remove_all(temp_file_path, ec);
      if (ec.failed()) {
        logger_->warn("fetch: failed to remove archive - {}", ec.message());
      }
      return result;
    }

    if (res.content_type == "application/octet-stream") {
      fs::rename(temp_file_path, output_path, ec);
      if (ec.failed()) {
        return outcome::success();  // TODO: ERROR
      }
      return outcome::success();
    }

    return outcome::success();  // TODO: ERROR
  }

  outcome::result<void> RemoteStore::deleteFromRemote(const std::string &url) {
    logger_->info("delete from remote: {}", url);

    OUTCOME_TRY(req, request_factory_->newRequest(url));

    req->setupMethod(ReqMethod::DELETE);

    req->setupHeaders(auth_headers_);

    auto res = req->perform();

    if (res.status_code != 200) {
      return outcome::success();  // TODO: ERROR
    }

    return outcome::success();
  }

}  // namespace fc::sector_storage::stores
