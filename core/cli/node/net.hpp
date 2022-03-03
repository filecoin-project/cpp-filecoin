/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cli/node/node.hpp"

namespace fc::cli::_node {
  inline void printPeer(const PeerInfo &peer) {
    for (const auto &addr : peer.addresses) {
      fmt::print("{}/p2p/{}\n", addr.getStringAddress(), peer.id.toBase58());
    }
  }

  struct Node_net_connect : Empty {
    CLI_RUN() {
      Node::Api api{argm};
      for (const auto &arg : argv) {
        const auto peer{cliArgv<PeerInfo>(arg, "peer")};
        cliTry(api->NetConnect(peer));
      }
    }
  };

  struct Node_net_listen : Empty {
    CLI_RUN() {
      Node::Api api{argm};
      const auto peer{cliTry(api->NetAddrsListen())};
      printPeer(peer);
    }
  };

  struct Node_net_peers : Empty {
    CLI_RUN() {
      Node::Api api{argm};
      const auto peers{cliTry(api->NetPeers())};
      for (const auto &peer : peers) {
        printPeer(peer);
      }
    }
  };
}  // namespace fc::cli::_node
