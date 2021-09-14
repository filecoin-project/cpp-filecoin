/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <spdlog/fmt/fmt.h>
#include <boost/filesystem/string_file.hpp>
#include <libp2p/multi/multiaddress.hpp>

#include "common/outcome2.hpp"

namespace fc::api::rpc {
  using libp2p::multi::Multiaddress;

  inline Outcome<std::pair<Multiaddress, std::string>> loadInfo(
      const boost::filesystem::path &repo, std::string_view env) {
    std::string address, token;
    if (auto _info{getenv(env.data())}) {
      std::string_view info{_info};
      auto i{info.find(":")};
      if (i == info.npos) {
        return {};
      }
      address = info.substr(0, i);
      token = info.substr(i + 1);
    } else if (!repo.empty()) {
      boost::filesystem::load_string_file(repo / "api", address);
      boost::filesystem::load_string_file(repo / "token", token);
    } else {
      return {};
    }
    OUTCOME_TRY(_address, Multiaddress::create(address));
    return std::make_pair(std::move(_address), std::move(token));
  }

  // TODO: hostname
  inline void saveInfo(const boost::filesystem::path &repo,
                       int port,
                       const boost::optional<std::string> &maybe_token) {
    auto address{fmt::format("/ip4/127.0.0.1/tcp/{}/http", port)};
    boost::filesystem::save_string_file(repo / "api", address);
    if (maybe_token.has_value())
      boost::filesystem::save_string_file(repo / "token", *maybe_token);
  }
}  // namespace fc::api::rpc
