/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cli/node/node.hpp"

namespace fc::cli::_node {
  struct Node_net_listen : Empty {
    CLI_RUN() {
      Node::Api api{argm};
      const auto peer{cliTry(api._->NetAddrsListen())};
      for (const auto &addr : peer.addresses) {
        fmt::print("{}\n", addr.getStringAddress());
      }
    }
  };
}  // namespace fc::cli::_node
