/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/common_api.hpp"

namespace fc::api {

  class NetworkApi : public CommonApi {
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


    template <typename A, typename F>
    void visitNet(A &&a, const F &f) {
      f(a.NetAddrsListen);
      f(a.NetConnect);
      f(a.NetPeers);
    }
  }

};  // namespace fc::api
