/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/peer_info.hpp>

#include "cli/validate/with.hpp"

namespace libp2p::peer {
  CLI_VALIDATE(PeerInfo) {
    fc::validateWith(out, values, [](const std::string &value) {
      auto address{multi::Multiaddress::create(value).value()};
      auto peer{address.getPeerId()};
      if (!peer) {
        throw std::exception{};
      }
      auto id{PeerId::fromBase58(*peer).value()};
      return PeerInfo{std::move(id), {std::move(address)}};
    });
  }
}  // namespace libp2p::peer
