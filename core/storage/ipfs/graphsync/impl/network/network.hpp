/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_NETWORK_HPP
#define CPP_FILECOIN_GRAPHSYNC_NETWORK_HPP

#include <functional>

#include <libp2p/host/host.hpp>
#include <libp2p/protocol/common/scheduler.hpp>

#include "common/logger.hpp"
#include "network_fwd.hpp"

namespace fc::storage::ipfs::graphsync {

  class Network : public std::enable_shared_from_this<Network>,
                  public PeerToNetworkFeedback {
   public:
    Network(std::shared_ptr<libp2p::Host> host,
            std::shared_ptr<libp2p::protocol::Scheduler> scheduler);

    ~Network() override;

    /// Starts accepting streams
    void start(std::shared_ptr<PeerToGraphsyncFeedback> feedback);

    void stop();

    bool canSendRequest(const PeerId &peer);

    void makeRequest(const PeerId &peer,
                     boost::optional<libp2p::multi::Multiaddress> address,
                     RequestId request_id,
                     SharedData request_body);

    void cancelRequest(RequestId request_id, SharedData request_body);

    bool addBlockToResponse(const PeerId &peer,
                            RequestId request_id,
                            const CID &cid,
                            const common::Buffer &data);

    void sendResponse(const PeerId &peer,
                      RequestId request_id,
                      ResponseStatusCode status,
                      const ResponseMetadata &metadata);

   private:
    void peerClosed(const PeerId &peer, ResponseStatusCode status) override;

    PeerContextPtr findContext(const PeerId &peer, bool create_if_not_found);

    void tryConnect(const PeerContextPtr &ctx);

    void onStreamAccepted(outcome::result<StreamPtr> rstream);

    /// Closes all streams gracefully
    void closeAllPeers();

    void asyncFeedback(const PeerId &peer,
                       int request_id,
                       ResponseStatusCode status);

    std::shared_ptr<libp2p::Host> host_;
    std::shared_ptr<libp2p::protocol::Scheduler> scheduler_;
    libp2p::peer::Protocol protocol_id_;

    std::shared_ptr<PeerToGraphsyncFeedback> feedback_;

    /// Peer set, where item can be found by const PeerID& (see operators <)
    using PeerSet = std::set<PeerContextPtr, std::less<>>;

    /// Set of peers, where item can be found by const PeerID&
    PeerSet peers_;

    std::map<RequestId, PeerContextPtr> active_requests_per_peer_;

    bool started_ = false;
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_NETWORK_HPP
