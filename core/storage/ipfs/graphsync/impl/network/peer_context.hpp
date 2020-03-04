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
                PeerToNetworkFeedback &network_feedback,
                libp2p::protocol::Scheduler &scheduler);

    ~PeerContext() override;

    /// Remote peer
    const PeerId peer;

    /// String representation for loggers and debug purposes
    const std::string str;

    void setOutboundAddress(
        boost::optional<libp2p::multi::Multiaddress> connect_to);

    bool needToConnect();

    enum State { can_connect, is_connecting, is_connected, is_closed };

    State getState() const;

    libp2p::peer::PeerInfo getOutboundPeerInfo() const;

    void onStreamConnected(outcome::result<StreamPtr> rstream);

    void onStreamAccepted(StreamPtr stream);

    void enqueueRequest(RequestId request_id, SharedData request_body);

    void cancelRequest(RequestId request_id, SharedData request_body);

    bool addBlockToResponse(RequestId request_id,
                            const CID &cid,
                            const common::Buffer &data);

    void sendResponse(RequestId request_id,
                      ResponseStatusCode status,
                      const ResponseMetadata &metadata);

    void close(ResponseStatusCode status);

   private:
    struct StreamCtx {
      std::unique_ptr<MessageReader> reader;
      std::shared_ptr<MessageQueue> queue;
      std::set<RequestId> remote_request_ids;
      std::unique_ptr<InboundEndpoint> response_endpoint;
      uint64_t expire_time = 0;
    };

    using Streams = std::map<StreamPtr, StreamCtx>;

    void onReaderEvent(const StreamPtr &stream,
                       outcome::result<Message>) override;

    void onWriterEvent(const StreamPtr &stream,
                       outcome::result<void> result) override;


    void onNewStream(StreamPtr stream);

    void closeStream(StreamPtr stream, ResponseStatusCode status);

    void onResponse(Message::Response &response);

    void onRequest(const StreamPtr &stream, Message::Request &request);

    void createMessageQueue(const StreamPtr &stream, StreamCtx &ctx);

    void createResponseEndpoint(const StreamPtr &stream, StreamCtx &ctx);

    void shiftExpireTime(StreamCtx &ctx);

    void shiftExpireTime(const StreamPtr &stream);

    void closeLocalRequests(ResponseStatusCode status);

    void onStreamCleanupTimer();

    Streams::iterator findResponseSink(RequestId request_id);

    PeerToGraphsyncFeedback &graphsync_feedback_;

    PeerToNetworkFeedback &network_feedback_;

    Scheduler &scheduler_;

    boost::optional<libp2p::multi::Multiaddress> connect_to_;

    std::unique_ptr<OutboundEndpoint> requests_endpoint_;

    std::set<RequestId> local_request_ids_;

    std::map<RequestId, StreamPtr> remote_requests_streams_;

    Streams streams_;

    Scheduler::Handle timer_;

    bool closed_ = false;

    ResponseStatusCode close_status_ = RS_INTERNAL_ERROR;
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_PEER_CONTEXT_HPP
