/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/basic/scheduler.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <unordered_set>

#include "node/events_fwd.hpp"

namespace fc::sync {
  using libp2p::basic::Scheduler;

  // Peer discovery wrapper, connects to peers in background
  class PeerDiscovery : public std::enable_shared_from_this<PeerDiscovery> {
   public:
    PeerDiscovery(
        std::shared_ptr<libp2p::Host> host,
        std::shared_ptr<Scheduler> scheduler,
        std::shared_ptr<libp2p::protocol::kademlia::Kademlia> kademlia);

    void start(events::Events &events);

   private:
    void makeRequest(libp2p::peer::PeerId peer_id);

    void onPossibleConnection(const libp2p::peer::PeerId &peer_id);

    void onPeerResolved(const libp2p::peer::PeerInfo &peer_info);

    void onTimer();

    std::shared_ptr<libp2p::Host> host_;
    std::shared_ptr<Scheduler> scheduler_;
    std::shared_ptr<libp2p::protocol::kademlia::Kademlia> kademlia_;
    libp2p::common::Hash256 this_node_id_;
    events::Connection peer_connected_event_;
    events::Connection peer_disconnected_event_;
    events::Connection block_pubsub_event_;
    events::Connection message_pubsub_event_;
    size_t n_connections_ = 0;
    std::unordered_set<libp2p::peer::PeerId> requests_in_progress_;
  };
}  // namespace fc::sync
