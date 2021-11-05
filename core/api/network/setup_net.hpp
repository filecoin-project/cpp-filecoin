/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/host/host.hpp>
#include "api/network/network_api.hpp"

namespace fc::api {
  using libp2p::Host;

  inline void fillNetApi(const std::shared_ptr<NetworkApi> &api,
                         const PeerInfo &api_peer_info,
                         const std::shared_ptr<Host> &host,
                         const common::Logger &logger) {
    api->NetAddrsListen = [api_peer_info]() -> outcome::result<PeerInfo> {
      return api_peer_info;
    };

    api->NetConnect = [=](auto &peer) {
      host->connect(peer);
      return outcome::success();
    };

    api->NetPeers = [=]() -> outcome::result<std::vector<PeerInfo>> {
      const auto &peer_repository = host->getPeerRepository();
      const auto connections =
          host->getNetwork().getConnectionManager().getConnections();
      std::vector<PeerInfo> result;
      for (const auto &conncection : connections) {
        const auto remote = conncection->remotePeer();
        if (remote.has_error()) {
          logger->error("get remote peer error", remote.error().message());
          continue;
        }
        result.push_back(peer_repository.getPeerInfo(remote.value()));
      }
      return result;
    };

    api->NetDisconnect = [=](const auto &peer) {
      host->disconnect(peer.id);
      return outcome::success();
    };
  }

}  // namespace fc::api
