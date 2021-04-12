/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/log/configurator.hpp>
#include <libp2p/log/logger.hpp>

namespace fc {
  // default libp2p logger config, libp2p won't work without it
  inline void libp2pSoralog() {
    static auto done{false};
    if (!done) {
      done = true;
      auto log{std::make_shared<soralog::LoggingSystem>(
          std::make_shared<libp2p::log::Configurator>(R"(
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
          )"))};
      auto result{log->configure()};
      assert(!result.has_error);
      assert(!result.has_warning);
      libp2p::log::setLoggingSystem(log);
    }
  }
}  // namespace fc
