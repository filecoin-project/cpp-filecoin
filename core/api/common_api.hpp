/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/peer_info.hpp>

#include "api/utils.hpp"
#include "common/buffer.hpp"

namespace fc::api {
  using common::Buffer;
  using libp2p::peer::PeerInfo;

  struct CommonApi {
    API_METHOD(AuthNew, Buffer, const std::vector<std::string> &)

    API_METHOD(NetAddrsListen, PeerInfo)

    API_METHOD(NetConnect, void, const PeerInfo &)

    API_METHOD(NetPeers, std::vector<PeerInfo>)

    API_METHOD(Version, VersionResult)
  };
}  // namespace fc::api
