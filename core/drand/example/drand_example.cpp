/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/asio/io_context.hpp>

#include "clock/time.hpp"
#include "common/logger.hpp"
#include "common/outcome_fmt.hpp"
#include "drand/impl/http.hpp"

int main(int argc, char *argv[]) {
  std::string host{argc > 1 ? argv[1] : "api.drand.sh"};
  spdlog::info("host: {}", host);
  boost::asio::io_context io;
  fc::drand::ChainInfo info;
  fc::drand::http::getInfo(io, host, [&](auto _info) {
    if (!_info.has_value()) {
      return spdlog::error("getInfo: {:#}", _info.error());
    }
    info = std::move(_info.value());
    fc::drand::http::getEntry(io, host, 0, [&](auto _latest) {
      if (!_latest) {
        return spdlog::error("getEntry: {}", _latest.error());
      }
      auto &latest{_latest.value()};
      spdlog::info("  public key: {}", info.key);
      spdlog::info("  genesis time: {}",
                   fc::clock::unixTimeToString(info.genesis));
      spdlog::info("  period: {}s", info.period.count());
      spdlog::info("  latest round: {}", latest.round);
      spdlog::info("  latest signature: {}", latest.signature);
      spdlog::info("  chain size: {}MB", (128 + latest.round * 96) >> 20);
    });
  });
  io.run();
}
