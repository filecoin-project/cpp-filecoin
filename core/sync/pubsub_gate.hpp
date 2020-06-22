/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_PUBSUB_GATE_HPP
#define CPP_FILECOIN_SYNC_PUBSUB_GATE_HPP

#include <boost/signals2.hpp>
#include <libp2p/protocol/gossip/gossip.hpp>

#include "common.hpp"

namespace fc::clock {
  class UTCClock;
}

namespace fc::sync {

  using Gossip = libp2p::protocol::gossip::Gossip;

  class PubSubGate : public std::enable_shared_from_this<PubSubGate> {
   public:
    using connection_t = boost::signals2::connection;

    explicit PubSubGate(std::shared_ptr<clock::UTCClock> clock,
                        std::shared_ptr<Gossip> gossip);

    void start(const std::string &network_name,
               const std::string &this_node_name);

    /// Unsubscribes from everything
    void stop();

    using OnBlockAvailable = void(const PeerId &from, const BlockMsg &msg);
    connection_t subscribeToBlocks(
        const std::function<OnBlockAvailable> &subscriber);

    using OnHeadAvailable = void(const PeerId &from, const HeadMsg &msg);
    connection_t subscribeToHeads(
        const std::function<OnHeadAvailable> &subscriber);

    /// If SECP message then signature is not null, and signed message may
    /// be composed by consumer
    using OnMessageAvailable = void(
        const PeerId &from,
        const UnsignedMessage &msg,
        boost::optional<std::reference_wrapper<const Signature>> signature);

    connection_t subscribeToMessages(
        const std::function<OnMessageAvailable> &subscriber);

    outcome::result<void> publishHead(const Tipset &tipset,
                                      const BigInt &weight);
    outcome::result<void> publishBlock(const BlockHeader &header,
                                       const std::vector<CID> &bls_msgs,
                                       const std::vector<CID> &secp_msgs);

   private:
    using GossipSubscription = libp2p::protocol::Subscription;
    using Bytes = std::vector<uint8_t>;

    bool started();

    uint64_t clockNano();

    // gossip callbacks. They decode raw data and broadcast

    void onBlock(const PeerId &from, const Bytes &raw);
    void onHead(const PeerId &from, const Bytes &raw);
    void onMsg(const PeerId &from, const Bytes &raw);

    std::shared_ptr<clock::UTCClock> clock_;
    std::shared_ptr<Gossip> gossip_;
    std::string this_node_name_;
    uint64_t nonce_ = -1;

    // signals broadcast decoded messages
    boost::signals2::signal<OnBlockAvailable> blocks_signal_;
    boost::signals2::signal<OnHeadAvailable> heads_signal_;
    boost::signals2::signal<OnMessageAvailable> msgs_signal_;

    // subscriptions receive raw data
    GossipSubscription blocks_subscription_;
    GossipSubscription heads_subscription_;
    GossipSubscription msgs_subscription_;

    // topic names
    std::string blocks_topic_;
    std::string heads_topic_;
    std::string msgs_topic_;
  };

}  // namespace fc::sync

#endif  // CPP_FILECOIN_SYNC_PUBSUB_GATE_HPP
