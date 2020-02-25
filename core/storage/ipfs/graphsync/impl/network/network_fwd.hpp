/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_NETWORK_FWD_HPP
#define CPP_FILECOIN_GRAPHSYNC_NETWORK_FWD_HPP

#include "storage/ipfs/graphsync/impl/common.hpp"

namespace libp2p::connection {
  class Stream;
}

namespace fc::storage::ipfs::graphsync {

  using StreamPtr = std::shared_ptr<libp2p::connection::Stream>;

  struct PeerContext;
  using PeerContextPtr = std::shared_ptr<PeerContext>;

  /// Operators needed to find a ctx in a set by peer id
  bool operator<(const PeerContextPtr &ctx, const PeerId &peer);
  bool operator<(const PeerId &peer, const PeerContextPtr &ctx);
  bool operator<(const PeerContextPtr &a, const PeerContextPtr &b);

  constexpr std::string_view kProtocolVersion = "/ipfs/graphsync/1.0.0";

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_NETWORK_FWD_HPP
