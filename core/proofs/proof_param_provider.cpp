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
#include "boost/asio/connect.hpp"
#include "boost/asio/ip/tcp.hpp"
#include "boost/beast/core.hpp"
#include "boost/beast/http.hpp"
#include "boost/beast/version.hpp"
#include "boost/filesystem.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/property_tree/json_parser.hpp"
#include "common/logger.hpp"
#include "common/outcome.hpp"
#include "crypto/blake2/blake2b160.hpp"
#include "proofs/proof_param_provider_error.hpp"

namespace fc::proofs {

  boost::mutex ProofParamProvider::fetch_mutex_ = boost::mutex();

  struct ResponseParseUrl {
    std::string host;
    std::string target;
  };

  bool hasSuffix(std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length()) {
      return (fullString.compare(fullString.length() - ending.length(),
                                 ending.length(),
                                 ending)
              == 0);
    } else {
      return false;
    }
  }

  outcome::result<ResponseParseUrl> parseUrl(const std::string &url_str) {
    ResponseParseUrl response{};

    std::smatch match;
    std::regex reg(
        "(https|http):\\/\\/([A-Za-z0-9\\-.]+)(\\/[\\/A-Za-z0-9\\-.]+)");

    if (std::regex_match(url_str, match, reg)) {
      response.host = match[2];
      response.target = match[3];
    } else {
      return ProofParamProviderError::INVALID_URL;
    }

    return response;
  }

  auto const default_gateway = "https://ipfs.io/ipfs/";
  auto const param_dir = "/var/tmp/filecoin-proof-parameters";
  auto const dir_env = "FIL_PROOFS_PARAMETER_CACHE";

  namespace beast = boost::beast;  // from <boost/beast.hpp>
  namespace http = beast::http;    // from <boost/beast/http.hpp>
  namespace net = boost::asio;     // from <boost/asio.hpp>
  using tcp = net::ip::tcp;        // from <boost/asio/ip/tcp.hpp>

  outcome::result<void> doFetch(const std::string &out, ParamFile info) {
    try {
      std::string gateway = default_gateway;
      if (char *custom_gateway = std::getenv("IPFS_GATEWAY")) {
        gateway = custom_gateway;
      }
      OUTCOME_TRY(url, parseUrl(gateway));
      auto const host = url.host;
      auto target = url.target;
      auto const port = "80";
      int version = 11;

      boost::uintmax_t f_size = boost::filesystem::exists(out)
                                    ? boost::filesystem::file_size(out)
                                    : 0;

      // The io_context is required for all I/O
      net::io_context ioc;

      // These objects perform our I/O
      tcp::resolver resolver(ioc);
      beast::tcp_stream stream(ioc);

      // Look up the domain name
      auto const results = resolver.resolve(host, port);

      // Make the connection on the IP address we get from a lookup
      stream.connect(results);

      if (target[target.size() - 1] != '/') {
        target += "/";
      }

      target += info.cid;

      // Set up an HTTP GET request message
      http::request<http::string_body> req{http::verb::get, target, version};
      req.set(http::field::host, host);
      req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
      std::string v = "bytes=" + std::to_string(f_size) + "-";
      req.insert("Range", v);
      // Send the HTTP request to the remote host
      http::write(stream, req);

      // This buffer is used for reading and must be persisted
      beast::flat_buffer buffer;

      // Declare a container to hold the response
      http::response_parser<http::file_body> res;
      res.body_limit((std::numeric_limits<std::uint64_t>::max)());
      boost::system::error_code ec_file;
      // Write the message to standard out
      res.get().body().open(
          out.c_str(), boost::beast::file_mode::append, ec_file);
      // Receive the HTTP response
      http::read(stream, buffer, res);
      // Gracefully close the socket
      beast::error_code ec;
      stream.socket().shutdown(tcp::socket::shutdown_both, ec);

      // not_connected happens sometimes
      // so don't bother reporting it.
      //
      if (ec && ec != beast::errc::not_connected) throw beast::system_error{ec};

      // If we get here then the connection is closed gracefully
    } catch (std::exception const &e) {
      std::cerr << "Error: " << e.what() << std::endl;
      return ProofParamProviderError::FAILED_DOWNLOADING;
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
      return ProofParamProviderError::CANNOT_CREATE_DIR;
    }

    std::vector<std::thread> threads;
    for (const auto &param_file : param_files) {
      if (param_file.sector_size != storage_size
          && hasSuffix(param_file.name, ".params")) {
        continue;
      }

      std::thread t(fetch, param_file);

      threads.push_back(std::move(t));
    }

    for (auto &th : threads) {
      th.join();
    }

    return outcome::success();
  }

  outcome::result<void> checkFile(const std::string &path,
                                  const ParamFile &info) {
    char *res = std::getenv("TRUST_PARAMS");
    if (res && std::strcmp(res, "1") == 0) {
      // Assuming parameter files are ok. DO NOT USE IN PRODUCTION
      return outcome::success();
    }

    std::ifstream ifs(path, std::ios::binary);

    if (!ifs.is_open()) return ProofParamProviderError::FILE_DOES_NOT_OPEN;

    auto sum = crypto::blake2b::blake2b_512_from_file(ifs);

    auto our_some = common::hex_lower(gsl::span<uint8_t>(sum.data(), 16));
    if (our_some != info.digest) {
      return ProofParamProviderError::CHECKSUM_MISMATCH;
    }

    return outcome::success();
  }

  void ProofParamProvider::fetch(const ParamFile &info) {
    auto path = boost::filesystem::path(getParamDir())
                / boost::filesystem::path(info.name);

    auto logger = common::createLogger("proofs params");
    logger->info("Fetch " + info.name);
    auto res = checkFile(path.string(), info);
    if (!res.has_error()) {
      logger->info(info.name + " already uploaded");
      return;
    } else if (boost::filesystem::exists(path)) {
      logger->warn(res.error().message());
    }

    fetch_mutex_.lock();

    auto fetch_res = doFetch(path.string(), info);

    if (fetch_res.has_error()) {
      logger->error(res.error().message());
      fetch_mutex_.unlock();
      return;
    }

    res = checkFile(path.string(), info);

    if (res.has_error()) {
      logger->error(res.error().message());
      boost::filesystem::remove(path);
      fetch_mutex_.unlock();
      return;
    }

    logger->info(info.name + " uploaded successfully");
    fetch_mutex_.unlock();
  }

  namespace pt = boost::property_tree;

  template <typename T>
  outcome::result<std::decay_t<T>> ensure(boost::optional<T> opt_entry) {
    if (not opt_entry) {
      return ProofParamProviderError::MISSING_ENTRY;
    }
    return opt_entry.value();
  }

  outcome::result<std::vector<ParamFile>> ProofParamProvider::readJson(
      const std::string &path) {
    pt::ptree tree;
    try {
      pt::read_json(path, tree);
    } catch (pt::json_parser_error &e) {
      return ProofParamProviderError::INVALID_JSON;
    }

    std::vector<ParamFile> result = {};

    for (const auto &elem : tree) {
      std::string some = elem.first;
      OUTCOME_TRY(cid, ensure(elem.second.get_child_optional("cid")));
      OUTCOME_TRY(digest, ensure(elem.second.get_child_optional("digest")));
      OUTCOME_TRY(sector_size,
                  ensure(elem.second.get_child_optional("sector_size")));

      ParamFile param_file;
      param_file.name = elem.first;
      param_file.cid = cid.data();
      param_file.digest = digest.data();
      try {
        param_file.sector_size =
            boost::lexical_cast<uint64_t>(sector_size.data());
      } catch (const boost::bad_lexical_cast &e) {
        return ProofParamProviderError::INVALID_SECTOR_SIZE;
      }

      result.push_back(param_file);
    }

    return result;
  }
}  // namespace fc::proofs
