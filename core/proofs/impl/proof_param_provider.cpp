/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proofs/proof_param_provider.hpp"

#include <cstdlib>
#include <regex>
#include <string>
#include <thread>

#include <curl/curl.h>
#include <boost/algorithm/string.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/filesystem.hpp>
#include <iostream>

#include "api/rpc/json.hpp"
#include "codec/json/json.hpp"
#include "common/file.hpp"
#include "common/outcome.hpp"
#include "crypto/blake2/blake2b160.hpp"
#include "proofs/proof_param_provider_error.hpp"

namespace fc::proofs {

  common::Logger logger = common::createLogger("proofs params");

  auto write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
    return fwrite(ptr, size, nmemb, (FILE *)stream);
  }

  auto const default_gateway = "https://proofs.filecoin.io/ipfs/";
  auto const param_dir = "/var/tmp/filecoin-proof-parameters";
  auto const dir_env = "FIL_PROOFS_PARAMETER_CACHE";

  std::string getParamDir() {
    if (char *dir = std::getenv(dir_env)) return dir;

    return param_dir;
  }

  outcome::result<void> doFetch(const std::string &out, const ParamFile &info) {
    try {
      /* init the curl session */
      auto curl_handle = curl_easy_init();

      std::string gateway = default_gateway;
      if (auto custom_gateway = std::getenv("IPFS_GATEWAY")) {
        gateway = custom_gateway;
      }

      if (gateway[gateway.size() - 1] != '/') {
        gateway += "/";
      }

      gateway += info.cid;

      /* set URL to get here */
      curl_easy_setopt(curl_handle, CURLOPT_URL, gateway.c_str());

      /* Switch off full protocol/debug output while testing, set to 1L to
       * enable it */
      curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0L);

      /* enable progress meter, set to 1L to disable it */
      curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);

      /* send all data to this function  */
      curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);

      boost::uintmax_t f_size = boost::filesystem::exists(out)
                                    ? boost::filesystem::file_size(out)
                                    : 0;

      struct curl_slist *chunk = NULL;

      std::string header = "Range: bytes=" + std::to_string(f_size) + "-";

      chunk = curl_slist_append(chunk, header.c_str());

      curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, chunk);

      /* open the file */
      auto pagefile = fopen(out.c_str(), "ab");
      if (pagefile) {
        /* write the page body to this file handle */
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, pagefile);

        /* get it! */
        curl_easy_perform(curl_handle);

        /* close the header file */
        fclose(pagefile);
      }

      /* cleanup curl stuff */
      curl_easy_cleanup(curl_handle);

      curl_slist_free_all(chunk);

    } catch (std::exception const &e) {
      logger->error("Error {}: {}", out, std::string(e.what()));
      return ProofParamProviderError::kFailedDownloading;
    }
    return outcome::success();
  }

  outcome::result<void> checkFile(const std::string &path,
                                  const ParamFile &info) {
    char *res = std::getenv("TRUST_PARAMS");
    if (res != nullptr && std::strcmp(res, "1") == 0) {
      // Assuming parameter files are ok. DO NOT USE IN PRODUCTION
      return outcome::success();
    }

    OUTCOME_TRY(sum, crypto::blake2b::blake2b_512_from_file(path));

    const auto our_some = common::hex_lower(gsl::make_span(sum).subspan(0, 16));
    if (our_some != info.digest) {
      return ProofParamProviderError::kChecksumMismatch;
    }

    return outcome::success();
  }

  outcome::result<void> fetch(const std::string &file_name,
                              const ParamFile &info) {
    auto path = boost::filesystem::path(getParamDir())
                / boost::filesystem::path(file_name);

    logger->info("Fetch " + file_name);
    auto res = checkFile(path.string(), info);
    if (!res.has_error()) {
      logger->info(file_name + " already downloaded");
      return outcome::success();
    }
    if (boost::filesystem::exists(path)) {
      logger->warn(res.error().message());
    }

    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);

    OUTCOME_TRY(doFetch(path.string(), info));

    res = checkFile(path.string(), info);

    if (res.has_error()) {
      logger->error("Failed {} check: {}", file_name, res.error().message());
      boost::filesystem::remove(path);
      return res.error();
    }

    logger->info(file_name + " downloaded successfully");
    return outcome::success();
  }

  outcome::result<void> getParams(const std::string &proof_param,
                                  uint64_t storage_size) {
    OUTCOME_TRY(data, common::readFile(proof_param));
    OUTCOME_TRY(jdoc, codec::json::parse(data));
    OUTCOME_TRY(param_files,
                api::decode<std::map<std::string, proofs::ParamFile>>(jdoc));

    boost::system::error_code ec{};
    boost::filesystem::create_directories(getParamDir(), ec);

    if (ec.failed()) {
      logger->error("Error: {}", ec.message());
      return ProofParamProviderError::kCannotCreateDir;
    }

    curl_global_init(CURL_GLOBAL_ALL);
    std::atomic<bool> errors = false;

    uint64_t cpus = std::thread::hardware_concurrency();
    if (cpus == 0) {
      cpus = sysconf(_SC_NPROCESSORS_ONLN);
    }

    auto io = std::make_shared<boost::asio::io_context>();
    for (const auto &entry : param_files) {
      if (entry.second.sector_size != storage_size
          && boost::ends_with(entry.first, ".params")) {
        continue;
      }
      io->post([entry, &errors]() {
        auto result = fetch(entry.first, entry.second);
        if (result.has_error() && not errors) {
          errors = true;
        }
      });
    }

    std::vector<std::thread> threads(cpus);
    for (size_t index = 0; index < cpus; index++) {
      threads[index] = std::thread([io]() { io->run(); });
    }

    for (auto &thread : threads) {
      thread.join();
    }
    curl_global_cleanup();

    if (not errors) return outcome::success();

    return ProofParamProviderError::kFailedDownloading;
  }

}  // namespace fc::proofs
