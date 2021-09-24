/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/peer_info.hpp>

#include "api/utils.hpp"
#include "api/version.hpp"
#include "common/buffer.hpp"
#include "primitives/jwt/jwt.hpp"

namespace fc::api {
  using common::Buffer;
  using libp2p::peer::PeerInfo;
  using primitives::jwt::Permission;
  namespace jwt = primitives::jwt;

  struct CommonApi {
    /**
     * Creates auth token to the remote connection
     * @return auth token
     */
    API_METHOD(AuthNew,
               jwt::kAdminPermission,
               Buffer,
               const std::vector<Permission> &)
    /**
     * Verify auth token
     * @return allow permissions
     */
    API_METHOD(AuthVerify,
               jwt::kReadPermission,
               std::vector<Permission>,
               const std::string &)

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

    API_METHOD(Version, jwt::kReadPermission, VersionResult)
  };

  template <typename A, typename F>
  void visitCommon(A &&a, const F &f) {
    f(a.AuthNew);
    f(a.AuthVerify);
    f(a.NetAddrsListen);
    f(a.NetConnect);
    f(a.NetPeers);
    f(a.Version);
  }
}  // namespace fc::api
