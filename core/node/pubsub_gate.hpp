/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/protocol/gossip/gossip.hpp>

#include "node/events.hpp"

namespace fc::clock {
  class UTCClock;
}

namespace fc::sync {

  using Gossip = libp2p::protocol::gossip::Gossip;

  class PubSubGate : public std::enable_shared_from_this<PubSubGate> {
   public:
    explicit PubSubGate(std::shared_ptr<Gossip> gossip);

    void start(const std::string &network_name,
               std::shared_ptr<events::Events> events);

    /// Unsubscribes from everything
    void stop();

    outcome::result<void> publish(const BlockWithCids &block);
    void publish(const SignedMessage &msg);

   private:
    using GossipSubscription = libp2p::protocol::Subscription;
    using Bytes = std::vector<uint8_t>;

    // Validation callbacks: decode, validate, and broadcast to signal
    // subscribers
    bool onBlock(const PeerId &from, const Bytes &raw);
    bool onMsg(const PeerId &from, const Bytes &raw);

    std::shared_ptr<Gossip> gossip_;

    std::shared_ptr<events::Events> events_;

    // subscriptions receive raw data
    GossipSubscription blocks_subscription_;
    GossipSubscription msgs_subscription_;

    // topic names
    std::string blocks_topic_;
    std::string msgs_topic_;

    events::Connection peer_connected_event_;
  };

}  // namespace fc::sync
