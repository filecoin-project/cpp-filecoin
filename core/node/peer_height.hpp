/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/bimap.hpp>
#include <boost/bimap/multiset_of.hpp>
#include <boost/bimap/unordered_set_of.hpp>

#include "node/events.hpp"

namespace fc::sync::peer_height {
  using primitives::ChainEpoch;

  struct PeerHeight {
    boost::bimap<boost::bimaps::unordered_set_of<PeerId, std::hash<PeerId>>,
                 boost::bimaps::multiset_of<ChainEpoch>>
        peers;
    events::Connection on_peer_head;
    events::Connection on_disconnect;

    PeerHeight(const std::shared_ptr<events::Events> &events) {
      on_peer_head =
          events->subscribePossibleHead([this](const events::PossibleHead &e) {
            if (e.source) {
              // note: may require successful fetch
              onHeight(*e.source, e.height);
            }
          });
      on_disconnect = events->subscribePeerDisconnected(
          [this](const events::PeerDisconnected &e) { onError(e.peer_id); });
    }

    void onHeight(const PeerId &peer, ChainEpoch height) {
      if (height <= 0) {
        return;
      }
      // note: may ignore peers behind our height
      const auto it{peers.left.find(peer)};
      if (it == peers.left.end()) {
        peers.insert({peer, height});
      } else if (height > it->second) {
        peers.left.replace_data(it, height);
      }
    }

    void onError(const PeerId &peer) {
      peers.left.erase(peer);
    }

    template <typename F>
    void visit(ChainEpoch min, const F &f) {
      for (auto it{peers.right.rbegin()}; it != peers.right.rend(); ++it) {
        if (it->first < min || !f(it->second)) {
          break;
        }
      }
    }
  };
}  // namespace fc::sync::peer_height
