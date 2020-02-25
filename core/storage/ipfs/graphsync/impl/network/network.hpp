/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_NETWORK_HPP
#define CPP_FILECOIN_GRAPHSYNC_NETWORK_HPP

#include <functional>

#include <libp2p/host/host.hpp>
#include <libp2p/protocol/common/scheduler.hpp>

#include "endpoint.hpp"
#include "storage/ipfs/graphsync/graphsync.hpp"

namespace fc::storage::ipfs::graphsync {

  class NetworkEvents {
   public:
    virtual ~NetworkEvents() = default;

    virtual void onResponse(const PeerId &peer,
                            int request_id,
                            ResponseStatusCode status,
                            ResponseMetadata metadata) = 0;

    virtual void onBlock(const PeerId &from, CID cid, common::Buffer data) = 0;

    virtual void onRemoteRequest(const PeerId &from,
                                 uint64_t tag,
                                 Message::Request request) = 0;
  };

  class Network : public std::enable_shared_from_this<Network>,
                  public EndpointEvents {
   public:
    Network(std::shared_ptr<libp2p::Host> host,
            std::shared_ptr<libp2p::protocol::Scheduler> scheduler);

    ~Network() override;

    /// Starts accepting streams
    void start(std::shared_ptr<NetworkEvents> feedback);

    void makeRequest(const PeerId &peer,
                     boost::optional<libp2p::multi::Multiaddress> address,
                     int request_id,
                     SharedData request_body);

    void cancelRequest(int request_id, SharedData request_body);

    void addBlockToResponse(const PeerId &peer,
                            uint64_t tag,
                            const CID &cid,
                            const common::Buffer &data);

    void sendResponse(const PeerId &peer,
                      uint64_t tag,
                      int request_id,
                      ResponseStatusCode status,
                      const ResponseMetadata &metadata);

   private:
    void onReadEvent(const PeerContextPtr &from,
                     uint64_t endpoint_tag,
                     outcome::result<Message> msg_res) override;

    void onWriteEvent(const PeerContextPtr &from,
                      uint64_t endpoint_tag,
                      outcome::result<void> result) override;

    void onEndpointError(const PeerContextPtr &from,
                         uint64_t endpoint_tag,
                         outcome::result<void> res);

    PeerContextPtr findOrCreateContext(const PeerId &peer);
    void tryConnect(PeerContextPtr ctx);

    void onStreamConnected(const PeerContextPtr &ctx,
                           outcome::result<StreamPtr> rstream);

    void onStreamAccepted(outcome::result<StreamPtr> rstream);

    uint64_t makeInboundEndpoint(const PeerContextPtr &ctx, StreamPtr stream);

    void incStreamRef(const StreamPtr &stream);
    void decStreamRef(const StreamPtr &stream);

    /// Closes all streams gracefully
    void closeAllStreams();

    void closeLocalRequestsForPeer(const PeerContextPtr &peer,
                                   ResponseStatusCode status);


    std::shared_ptr<libp2p::Host> host_;
    std::shared_ptr<libp2p::protocol::Scheduler> scheduler_;
    libp2p::peer::Protocol protocol_id_;

    std::shared_ptr<NetworkEvents> feedback_;

    /// Set of peers, where item can be found by const PeerID&
    std::set<PeerContextPtr, std::less<>> peers_;

    /// against reentrancy
    PeerContextPtr dont_connect_to_;

    std::map<StreamPtr, int> stream_refs_;

    std::map<int, PeerContextPtr> active_requests_;

    bool started_ = false;
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_NETWORK_HPP
