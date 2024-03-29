/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/utils.hpp"
#include "fwd.hpp"

namespace fc::api {
  namespace jwt = primitives::jwt;
  using libp2p::peer::PeerId;
  using libp2p::peer::PeerInfo;

  class NetworkApi {
   public:
    /**
     * Returns listen addresses.
     */
    API_METHOD(NetAddrsListen, jwt::kReadPermission, PeerInfo)

    /**
     * Initiates the connection to the peer.
     */
    API_METHOD(NetConnect, jwt::kWritePermission, void, const PeerInfo &)

    /**
     * Returns all peers connected to the this host.
     */
    API_METHOD(NetPeers, jwt::kReadPermission, std::vector<PeerInfo>)

    /**
     * Removes provided peer from list of connected.
     */
    API_METHOD(NetDisconnect, jwt::kWritePermission, void, const PeerInfo &)

    API_METHOD(NetProtectAdd,
               jwt::kAdminPermission,
               void,
               const std::vector<PeerId> &);
  };

  template <typename A, typename F>
  void visitNet(A &&a, const F &f) {
    f(a.NetAddrsListen);
    f(a.NetConnect);
    f(a.NetPeers);
    f(a.NetDisconnect);
    f(a.NetProtectAdd);
  }

};  // namespace fc::api
