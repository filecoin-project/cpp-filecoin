/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proofs/proof_param_provider.hpp"

#include <cstdlib>
#include <iostream>
#include <regex>
#include <string>
#include <thread>

#include <curl/curl.h>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "common/outcome.hpp"
#include "crypto/blake2/blake2b160.hpp"
#include "proofs/proof_param_provider_error.hpp"

namespace fc::proofs {

  common::Logger ProofParamProvider::logger_ =
      common::createLogger("proofs params");

  bool ProofParamProvider::errors_ = false;

  auto write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
    return fwrite(ptr, size, nmemb, (FILE *)stream);
  }

  auto const default_gateway = "https://ipfs.io/ipfs/";
  auto const param_dir = "/var/tmp/filecoin-proof-parameters";
  auto const dir_env = "FIL_PROOFS_PARAMETER_CACHE";

  outcome::result<void> ProofParamProvider::doFetch(const std::string &out,
                                                    const ParamFile &info) {
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
      logger_->error("Error: " + std::string(e.what()));
      return ProofParamProviderError::kFailedDownloading;
    }
    return outcome::success();
  }

  std::string getParamDir() {
    if (char *dir = std::getenv(dir_env)) return dir;

    return param_dir;
  }

  outcome::result<void> ProofParamProvider::getParams(
      const gsl::span<ParamFile> &param_files, uint64_t storage_size) {
    try {
      boost::filesystem::create_directories(getParamDir());
    } catch (const std::exception &e) {
      logger_->error("Error:" + std::string(e.what()));
      return ProofParamProviderError::kCannotCreateDir;
    }

    curl_global_init(CURL_GLOBAL_ALL);
    errors_ = false;
    std::vector<std::thread> threads;
    for (const auto &param_file : param_files) {
      if (param_file.sector_size != storage_size) {
        continue;
      }

      threads.emplace_back(std::thread{fetch, param_file});
    }

    for (auto &th : threads) {
      th.join();
    }
    curl_global_cleanup();

    if (!errors_) return outcome::success();

    return ProofParamProviderError::kFailedDownloading;
  }

  outcome::result<void> checkFile(const std::string &path,
                                  const ParamFile &info) {
    char *res = std::getenv("TRUST_PARAMS");
    if (res != nullptr && std::strcmp(res, "1") == 0) {
      // Assuming parameter files are ok. DO NOT USE IN PRODUCTION
      return outcome::success();
    }

    std::ifstream ifs(path, std::ios::binary);

    if (!ifs.is_open()) return ProofParamProviderError::kFileDoesNotOpen;

    auto sum = crypto::blake2b::blake2b_512_from_file(ifs);

    auto our_some = common::hex_lower(gsl::make_span(sum).subspan(0, 16));
    if (our_some != info.digest) {
      return ProofParamProviderError::kChecksumMismatch;
    }

    return outcome::success();
  }

  void ProofParamProvider::fetch(const ParamFile &info) {
    auto path = boost::filesystem::path(getParamDir())
                / boost::filesystem::path(info.name);

    logger_->info("Fetch " + info.name);
    auto res = checkFile(path.string(), info);
    if (!res.has_error()) {
      logger_->info(info.name + " already downloaded");
      return;
    }
    if (boost::filesystem::exists(path)) {
      logger_->warn(res.error().message());
    }

    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);

    auto fetch_res = doFetch(path.string(), info);

    if (fetch_res.has_error()) {
      errors_ = true;
      logger_->error(res.error().message());

      return;
    }

    res = checkFile(path.string(), info);

    if (res.has_error()) {
      errors_ = true;
      logger_->error(res.error().message());
      boost::filesystem::remove(path);

      return;
    }

    logger_->info(info.name + " downloaded successfully");
  }

  namespace pt = boost::property_tree;

  template <typename T>
  outcome::result<std::decay_t<T>> ensure(boost::optional<T> opt_entry) {
    if (not opt_entry) {
      return ProofParamProviderError::kMissingEntry;
    }
    return opt_entry.value();
  }

  outcome::result<std::vector<ParamFile>> ProofParamProvider::readJson(
      const std::string &path) {
    pt::ptree tree;
    try {
      pt::read_json(path, tree);
    } catch (pt::json_parser_error &e) {
      return ProofParamProviderError::kInvalidJSON;
    }

    std::vector<ParamFile> result = {};

    for (const auto &[name, elem] : tree) {
      OUTCOME_TRY(cid, ensure(elem.get_child_optional("cid")));
      OUTCOME_TRY(digest, ensure(elem.get_child_optional("digest")));
      OUTCOME_TRY(sector_size, ensure(elem.get_child_optional("sector_size")));

      ParamFile param_file;
      param_file.name = name;
      param_file.cid = cid.data();
      param_file.digest = digest.data();
      try {
        param_file.sector_size =
            boost::lexical_cast<uint64_t>(sector_size.data());
      } catch (const boost::bad_lexical_cast &e) {
        return ProofParamProviderError::kInvalidSectorSize;
      }

      result.push_back(param_file);
    }

    return result;
  }
}  // namespace fc::proofs
