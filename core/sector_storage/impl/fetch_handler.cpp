/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/fetch_handler.hpp"

#include <boost/beast/core/ostream.hpp>
#include <boost/filesystem.hpp>
#include <regex>
#include "api/rpc/json.hpp"
#include "codec/json/json.hpp"
#include "common/logger.hpp"
#include "common/tarutil.hpp"
#include "sector_storage/stores/store_error.hpp"

namespace fc::sector_storage {
  namespace http = api::http;
  using tcp = api::tcp;
  using primitives::StorageID;
  namespace fs = boost::filesystem;

  common::Logger server_logger = common::createLogger("remote server");

  template <typename T>
  void setupResponse(const http::request<http::dynamic_body> &request,
                     http::response<T> &response) {
    response.version(request.version());
    response.keep_alive(false);
  }

  api::WrapperResponse makeErrorResponse(
      const http::request<http::dynamic_body> &request, http::status status) {
    http::response<http::dynamic_body> response;
    setupResponse(request, response);
    response.result(status);
    return api::WrapperResponse(std::move(response));
  }

  api::WrapperResponse remoteStatFs(
      const http::request<http::dynamic_body> &request,
      const std::shared_ptr<stores::LocalStore> &local_store,
      const common::Logger &logger,
      const fc::primitives::StorageID &storage_id) {
    auto maybe_stat = local_store->getFsStat(storage_id);
    if (maybe_stat.has_error()) {
      if (maybe_stat
          == outcome::failure(stores::StoreErrors::kNotFoundStorage)) {
        return makeErrorResponse(request, http::status::not_found);
      }
      logger->error("Error remote get sector: {}",
                    maybe_stat.error().message());
      return makeErrorResponse(request, http::status::internal_server_error);
    }

    auto json_doc = fc::api::encode(maybe_stat.value());

    auto maybe_json = fc::codec::json::format(&json_doc);

    if (maybe_json.has_error()) {
      logger->error("Error remote get sector: {}",
                    maybe_json.error().message());
      return makeErrorResponse(request, http::status::internal_server_error);
    }

    http::response<http::dynamic_body> response;
    setupResponse(request, response);
    response.set(http::field::content_type, "application/json");
    boost::beast::ostream(response.body())
        .write(fc::common::span::bytestr(maybe_json.value().data()),
               maybe_json.value().size());
    return api::WrapperResponse(std::move(response));
  }

  api::WrapperResponse remoteGetSector(
      const http::request<http::dynamic_body> &request,
      const std::shared_ptr<stores::LocalStore> &local_store,
      const common::Logger &logger,
      const std::string &type,
      const std::string &sector) {
    auto maybe_sector = fc::primitives::sector_file::parseSectorName(sector);

    if (maybe_sector.has_error()) {
      logger->error("Error remote get sector: {}",
                    maybe_sector.error().message());
      return makeErrorResponse(request, http::status::internal_server_error);
    }

    auto maybe_type = fc::primitives::sector_file::fromString(type);
    if (maybe_type.has_error()) {
      logger->error("Error remote get sector: {}",
                    maybe_type.error().message());
      return makeErrorResponse(request, http::status::internal_server_error);
    }

    // Proof type is 0 because we don't allocate anything
    auto maybe_paths = local_store->acquireSector(
        maybe_sector.value(),
        static_cast<RegisteredProof>(0),
        maybe_type.value(),
        fc::primitives::sector_file::SectorFileType::FTNone,

        fc::sector_storage::stores::PathType::kStorage,
        fc::sector_storage::stores::AcquireMode::kMove);
    if (maybe_paths.has_error()) {
      logger->error("Error remote get sector: {}",
                    maybe_paths.error().message());
      return makeErrorResponse(request, http::status::internal_server_error);
    }

    auto maybe_path =
        maybe_paths.value().paths.getPathByType(maybe_type.value());
    if (maybe_path.has_error()) {
      logger->error("Error remote get sector: {}",
                    maybe_path.error().message());
      return makeErrorResponse(request, http::status::internal_server_error);
    }
    if (maybe_path.value().empty()) {
      logger->error("Error remote get sector: acquired path was empty");
      return makeErrorResponse(request, http::status::internal_server_error);
    }

    std::string response_file = maybe_path.value();

    std::function<void()> clear_function;
    http::response<http::file_body> response;
    setupResponse(request, response);
    if (boost::filesystem::is_directory(maybe_path.value())) {
      auto temp_file = fs::temp_directory_path() / fs::unique_path();
      clear_function = [temp_file]() {
        if (fs::exists(temp_file)) {
          fs::remove_all(temp_file);
        }
      };
      auto maybe_tar =
          fc::common::zipTar(maybe_path.value(), temp_file.string());
      if (maybe_tar.has_error()) {
        logger->error("Error remote get sector: {}",
                      maybe_tar.error().message());
        return makeErrorResponse(request, http::status::internal_server_error);
      }
      response_file = temp_file.string();
      response.set(http::field::content_type, "application/x-tar");
    } else {
      response.set(http::field::content_type, "application/octet-stream");
    }

    boost::system::error_code ec;
    response.body().open(
        response_file.c_str(), boost::beast::file_mode::scan, ec);
    if (ec.failed()) {
      logger->error("Error remote get sector: {}", ec.message());
      return makeErrorResponse(request, http::status::internal_server_error);
    }
    return api::WrapperResponse(std::move(response), std::move(clear_function));
  }

  api::WrapperResponse remoteRemoveSector(
      const http::request<http::dynamic_body> &request,
      const std::shared_ptr<stores::LocalStore> &local_store,
      const common::Logger &logger,
      const std::string &type,
      const std::string &sector) {
    auto maybe_sector = fc::primitives::sector_file::parseSectorName(sector);

    if (maybe_sector.has_error()) {
      logger->error("Error remote remove sector: {}",
                    maybe_sector.error().message());
      return makeErrorResponse(request, http::status::internal_server_error);
    }

    auto maybe_type = fc::primitives::sector_file::fromString(type);
    if (maybe_type.has_error()) {
      logger->error("Error remote remove sector: {}",
                    maybe_type.error().message());
      return makeErrorResponse(request, http::status::internal_server_error);
    }

    auto maybe_error =
        local_store->remove(maybe_sector.value(), maybe_type.value());
    if (maybe_error.has_error()) {
      logger->error("Error remote remove sector: {}",
                    maybe_error.error().message());
      return makeErrorResponse(request, http::status::internal_server_error);
    }

    http::response<http::dynamic_body> response;
    setupResponse(request, response);
    response.result(http::status::ok);
    return api::WrapperResponse(std::move(response));
  }

  api::RouteHandler serveHttp(
      const std::shared_ptr<stores::LocalStore> &local_store) {
    return [local = local_store, logger = server_logger](
               const http::request<http::dynamic_body> &request)
               -> api::WrapperResponse {
      // TODO(artyom-yurin): [FIL-259] Check token

      std::regex stat_rgx(R"(\/remote\/stat\/([\w-]+))");
      std::regex sector_rgx(R"(\/remote\/([\w]+)\/([\w-]+))");
      std::smatch matches;

      std::string target = request.target().to_string();

      switch (request.method()) {
        case http::verb::get:
          if (std::regex_search(
                  target.cbegin(), target.cend(), matches, stat_rgx)) {
            return remoteStatFs(request, local, logger, matches[1]);
          } else if (std::regex_search(
                         target.cbegin(), target.cend(), matches, sector_rgx)) {
            return remoteGetSector(
                request, local, logger, matches[1], matches[2]);
          } else {
            return makeErrorResponse(request,
                                     http::status::internal_server_error);
          }
        case http::verb::delete_:
          if (std::regex_search(
                  target.cbegin(), target.cend(), matches, sector_rgx)) {
            return remoteRemoveSector(
                request, local, logger, matches[1], matches[2]);
          } else {
            return makeErrorResponse(request,
                                     http::status::internal_server_error);
          }
        default:
          return makeErrorResponse(request, http::status::method_not_allowed);
      }
    };
  }

}  // namespace fc::sector_storage
