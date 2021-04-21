/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/peer_info.hpp>

#include "api/utils.hpp"
#include "api/version.hpp"
#include "common/buffer.hpp"

namespace fc::api {
  using common::Buffer;
  using libp2p::peer::PeerInfo;

  struct CommonApi {
    /**
     * Creates auth token to the remote connection
     * @returns auth token
     */
    API_METHOD(AuthNew, Buffer, const std::vector<std::string> &)

    /**
     * Returns listen addresses.
     */
    API_METHOD(NetAddrsListen, PeerInfo)

    /**
     * Initiates the connection to the peer.
     */
    API_METHOD(NetConnect, void, const PeerInfo &)

    /**
     * Returns all peers connected to the this host.
     */
    API_METHOD(NetPeers, std::vector<PeerInfo>)

    API_METHOD(Version, VersionResult)
  };
}  // namespace fc::api
