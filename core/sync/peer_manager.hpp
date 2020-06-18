/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_PEER_MANAGER_HPP
#define CPP_FILECOIN_SYNC_PEER_MANAGER_HPP

#include <set>
#include <unordered_map>

#include <libp2p/peer/peer_info.hpp>
#include "node/builder.hpp"
#include "node/config.hpp"
#include "hello.hpp"

namespace fc::sync {

  class PeerManager : public std::enable_shared_from_this<PeerManager> {
   public:
    using PeerId = libp2p::peer::PeerId;

    struct GetPeerOptions {
      bool must_be_network_node = false;
      bool must_be_connected = false;
      std::set<std::string> must_handle_protocols;
    };

    PeerManager(const node::NodeObjects &o, const node::Config &c);

    ~PeerManager();

    boost::optional<libp2p::peer::PeerInfo> getPeerInfo(
        const PeerId &peer_id) const;

    boost::optional<libp2p::peer::PeerInfo> getPeerInfo(
        const PeerId &peer_id, const GetPeerOptions &options) const;

    void addBootstrapPeer(const std::string &p2p_address);

    const std::vector<libp2p::peer::PeerInfo> &getBootstrapPeers() const;

    // chooses max weight peer from connected node peers
    boost::optional<PeerId> choosePeer();

    // connects to bootstrap peers
    outcome::result<void> start();

    // disconnects from peers
    void stop();

   private:
    void onIdentifyReceived(const PeerId& peer);
    void onHello(const PeerId &peer,
        outcome::result<Hello::Message> state);
    void onHelloLatencyMessage(const PeerId &peer,
                         outcome::result<uint64_t> result);

    struct InfoAndProtocols {
      primitives::BigInt current_weight;
      boost::optional<libp2p::multi::Multiaddress> connect_to;
      std::vector<std::string> protocols;
    };

    const std::set<std::string> node_protocols_;
    std::shared_ptr<libp2p::Host> host_;
    std::shared_ptr<clock::UTCClock> utc_clock_;
    std::shared_ptr<Hello> hello_;
    std::shared_ptr<libp2p::protocol::Identify> identify_protocol_;
    std::shared_ptr<libp2p::protocol::IdentifyPush> identify_push_protocol_;
    std::shared_ptr<libp2p::protocol::IdentifyDelta> identify_delta_protocol_;
    std::shared_ptr<storage::blockchain::ChainStore> chain_store_;
    std::vector<libp2p::peer::PeerInfo> bootstrap_peers_;
    std::unordered_map<PeerId, InfoAndProtocols> node_peers_;
    std::set<std::pair<primitives::BigInt, PeerId>, std::greater<>>
        node_peers_weighted_;
    std::unordered_map<PeerId, InfoAndProtocols> other_peers_;
    bool started_ = false;

    boost::signals2::connection on_identify_;
  };

}  // namespace fc::sync

#endif  // CPP_FILECOIN_SYNC_PEER_MANAGER_HPP
