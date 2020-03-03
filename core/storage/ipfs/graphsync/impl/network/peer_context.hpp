/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_PEER_CONTEXT_HPP
#define CPP_FILECOIN_GRAPHSYNC_PEER_CONTEXT_HPP

#include <map>
#include <set>

#include <libp2p/peer/peer_info.hpp>

#include "network_fwd.hpp"

namespace fc::storage::ipfs::graphsync {

  class OutboundEndpoint;
  class InboundEndpoint;
  class MessageReader;
  class MessageQueue;

  class PeerContext : public std::enable_shared_from_this<PeerContext>,
                      public EndpointToPeerFeedback {
   public:
    PeerContext(PeerContext &&) = delete;
    PeerContext(const PeerContext &) = delete;
    PeerContext &operator=(const PeerContext &) = delete;
    PeerContext &operator=(PeerContext &&) = delete;

    /// The only ctor, since PeerContext is to be used in shared_ptr only
    PeerContext(PeerId peer_id,
                PeerToGraphsyncFeedback &graphsync_feedback,
                PeerToNetworkFeedback &network_feedback);

    ~PeerContext() override;

    /// Remote peer
    const PeerId peer;

    /// String representation for loggers and debug purposes
    const std::string str;

    /// Creates requests endpoint if not exist, returns true if just created
    bool createRequestsEndpoint(
        boost::optional<libp2p::multi::Multiaddress> connect_to);

    bool isConnecting() const;

    bool canConnect() const;

    libp2p::peer::PeerInfo getOutboundPeerInfo() const;

    void onStreamConnected(outcome::result<StreamPtr> rstream);

    void onStreamAccepted(StreamPtr stream);

    outcome::result<void> enqueueRequest(RequestId request_id,
                                         SharedData request_body);

    void cancelRequest(RequestId request_id, SharedData request_body);

    bool addBlockToResponse(RequestId request_id,
                            const CID &cid,
                            const common::Buffer &data);

    void sendResponse(RequestId request_id,
                      ResponseStatusCode status,
                      const ResponseMetadata &metadata);

    void close();

   private:
    struct StreamCtx {
      std::unique_ptr<MessageReader> reader;
      std::shared_ptr<MessageQueue> queue;
      std::set<RequestId> remote_request_ids;
      std::unique_ptr<InboundEndpoint> response_endpoint;
    };

    using Streams = std::map<StreamPtr, StreamCtx>;

    void onReaderEvent(const StreamPtr &stream,
                       outcome::result<Message>) override;

    void onWriterEvent(const StreamPtr &stream,
                       outcome::result<void> result) override;

    void onNewStream(StreamPtr stream, bool connected);

    void closeStream(StreamPtr stream);

    void onResponse(Message::Response& response);

    void onRequest(const StreamPtr& stream, Message::Request& request);

    void createMessageQueue(const StreamPtr& stream, StreamCtx& ctx);

    void createResponseEndpoint(const StreamPtr& stream, StreamCtx& ctx);

    void checkIfClosable(const StreamPtr& stream, StreamCtx& ctx);

    void checkIfClosable(const StreamPtr &stream);

    void closeLocalRequests();

    Streams::iterator findResponseSink(RequestId request_id);

    PeerToGraphsyncFeedback &graphsync_feedback_;

    PeerToNetworkFeedback &network_feedback_;

    boost::optional<libp2p::multi::Multiaddress> connect_to_;

    std::unique_ptr<OutboundEndpoint> requests_endpoint_;

    std::set<RequestId> local_request_ids_;

    std::map<RequestId, StreamPtr> remote_requests_streams_;

    Streams streams_;

    bool can_connect_ = true;

    bool closed_ = false;
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_PEER_CONTEXT_HPP
