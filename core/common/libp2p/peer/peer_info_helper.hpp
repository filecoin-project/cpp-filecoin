/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_COMMON_LIBP2P_PEER_PEER_INFO_HELPER_HPP
#define CPP_FILECOIN_CORE_COMMON_LIBP2P_PEER_PEER_INFO_HELPER_HPP

#include <libp2p/peer/peer_info.hpp>
#include <sstream>

using libp2p::peer::PeerInfo;

/**
 * Represent peer info as human readable string
 * @param peer_info to prettify
 * @return string
 */
static std::string peerInfoToPrettyString(const PeerInfo &peer_info) {
  std::stringstream ss;
  for (const auto &address : peer_info.addresses) {
    ss << std::string(address.getStringAddress()) << " ";
  }
  return ss.str();
}

#endif  // CPP_FILECOIN_CORE_COMMON_LIBP2P_PEER_PEER_INFO_HELPER_HPP
