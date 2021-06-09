/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/log/configurator.hpp>
#include <libp2p/log/logger.hpp>

namespace fc {
  // default libp2p logger config, libp2p won't work without it
  inline void libp2pSoralog(std::string path = {}) {
    static auto done{false};
    if (!done) {
      done = true;
      std::string console{R"(
sinks:
  - name: console
    type: console
    color: true
groups:
  - name: main
    sink: console
    level: info
    children:
      - name: libp2p
          )"};
      std::string file{R"(
sinks:
  - name: file
    type: file
    path: {}
groups:
  - name: main
    sink: file
    level: info
    children:
      - name: libp2p
          )"};
      auto log{std::make_shared<soralog::LoggingSystem>(
          std::make_shared<libp2p::log::Configurator>(
              path.empty() ? console : fmt::format(file, path)))};
      auto result{log->configure()};
      assert(!result.has_error);
      assert(!result.has_warning);
      libp2p::log::setLoggingSystem(log);
    }
  }
}  // namespace fc
