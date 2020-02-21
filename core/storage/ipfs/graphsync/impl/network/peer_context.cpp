/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <spdlog/fmt/fmt.h>

#include "peer_context.hpp"

namespace fc::storage::ipfs::graphsync {

  namespace {

    /// String representation for loggers and debug purposes
    std::string makeStringRepr(uint64_t session_id, const PeerId &peer_id) {
      return fmt::format("{}-{}", session_id, peer_id.toBase58().substr(46));
    }

    std::string makeStringRepr(const PeerId &peer_id) {
      return peer_id.toBase58().substr(46);
    }

    /// Needed for sets and maps
    inline bool less(const PeerId &a, const PeerId &b) {
      // N.B. toVector returns const std::vector&, i.e. it is fast
      return a.toVector() < b.toVector();
    }

  }  // namespace

  PeerContext::PeerContext(PeerId peer_id)
      : peer(std::move(peer_id)),
        str(makeStringRepr(peer))
        {}

  bool operator<(const PeerContextPtr &ctx, const PeerId &peer) {
    if (!ctx)
      return false;
    return less(ctx->peer, peer);
  }

  bool operator<(const PeerId &peer, const PeerContextPtr &ctx) {
    if (!ctx)
      return false;
    return less(peer, ctx->peer);
  }

  bool operator<(const PeerContextPtr &a, const PeerContextPtr &b) {
    if (!a || !b)
      return false;
    return less(a->peer, b->peer);
  }

}  // namespace fc::storage::ipfs::graphsync
