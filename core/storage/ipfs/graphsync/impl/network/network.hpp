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

  /// Network part of graphsync component
  class Network : public std::enable_shared_from_this<Network>,
                  public PeerToNetworkFeedback {
   public:
    /// Ctor.
    /// \param host libp2p host object
    /// \param scheduler libp2p scheduler
    Network(std::shared_ptr<libp2p::Host> host,
            std::shared_ptr<libp2p::protocol::Scheduler> scheduler);

    ~Network() override;

    /// Starts accepting streams
    /// \param feedback Feedback interface of core component
    void start(std::shared_ptr<PeerToGraphsyncFeedback> feedback);

    /// Stops all network operations gracefully
    void stop();

    /// Called by core graphsync module to know if peer is requestable at the
    /// moment. Creates a peer context in the background, if needed
    /// \param peer peer ID
    /// \return true if request can be made now
    bool canSendRequest(const PeerId &peer);

    /// Makes a new reauest to peer
    /// \param peer peer ID
    /// \param address optional network address
    /// \param request_id request ID
    /// \param request_body serialized request data
    void makeRequest(const PeerId &peer,
                     boost::optional<libp2p::multi::Multiaddress> address,
                     RequestId request_id,
                     SharedData request_body);

    /// Cancels previously sent request
    /// \param request_id request ID
    /// \param request_body serialized request body
    void cancelRequest(RequestId request_id, SharedData request_body);

    /// Adds data block to response
    /// \param peer peer ID
    /// \param request_id request ID
    /// \param cid CID of the block
    /// \param data data block, raw bytes
    /// \return true if block is added to the response body
    /// and response object itself can be sent
    bool addBlockToResponse(const PeerId &peer,
                            RequestId request_id,
                            const CID &cid,
                            const common::Buffer &data);

    /// Sends response to peer. Data blocks may be added previously
    /// to this response
    /// \param peer peer ID
    /// \param request_id request ID
    /// \param status status code
    /// \param extensions - data for protocol extensions
    void sendResponse(const PeerId &peer,
                      RequestId request_id,
                      ResponseStatusCode status,
                      const std::vector<Extension> &extensions);

   private:
    /// Callback from peer context that it's closed
    /// \param peer peer ID
    /// \param status close reason as ResponseStatusCode
    void peerClosed(const PeerId &peer, ResponseStatusCode status) override;

    /// Finds peer context in peer set. Creates a new one if needed
    /// \param peer peer ID
    /// \param create_if_not_found if true, a new PeerContext will be created
    /// if needed
    /// \return PeerContext as shared_ptr
    PeerContextPtr findContext(const PeerId &peer, bool create_if_not_found);

    /// Tries to connect to peer
    /// \param ctx PeerContext
    void tryConnect(const PeerContextPtr &ctx);

    /// Libp2p network server callback
    /// \param rstream Accept result, contains a new inbound stream on success
    void onStreamAccepted(outcome::result<StreamPtr> rstream);

    /// Closes all peers gracefully
    void closeAllPeers();

    /// Asynchronously posts response status to local request callback, in the
    /// next reactor cycle. Used on errors to decouple callbacks asynchronously
    /// \param peer peer ID
    /// \param request_id request ID
    /// \param status status code
    void asyncFeedback(const PeerId &peer,
                       RequestId request_id,
                       ResponseStatusCode status);

    /// libp2p host object
    std::shared_ptr<libp2p::Host> host_;

    /// libp2p scheduler object
    std::shared_ptr<libp2p::protocol::Scheduler> scheduler_;

    /// libp2p peorocol ID
    libp2p::peer::Protocol protocol_id_;

    /// Feedback to graphsync core module (owning object)
    std::shared_ptr<PeerToGraphsyncFeedback> feedback_;

    /// Peer set, where item can be found by const PeerID& (see operators <)
    using PeerSet = std::set<PeerContextPtr, std::less<>>;

    /// Set of peers, where item can be found by const PeerID&
    PeerSet peers_;

    /// Keeps tracks of active outbound requests, maps them to peers
    std::map<RequestId, PeerContextPtr> active_requests_per_peer_;

    /// Indicates if the node is active
    bool started_ = false;
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_NETWORK_HPP
