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

  /// Peer context, used by Network module to communicate wit individual peer,
  /// manages peer's streams and their logic
  class PeerContext : public std::enable_shared_from_this<PeerContext>,
                      public EndpointToPeerFeedback {
   public:
    PeerContext(PeerContext &&) = delete;
    PeerContext(const PeerContext &) = delete;
    PeerContext &operator=(const PeerContext &) = delete;
    PeerContext &operator=(PeerContext &&) = delete;

    /// The only ctor, since PeerContext is to be used in shared_ptr only
    /// \param peer_id peer ID
    /// \param graphsync_feedback feedback interface of core module
    /// \param network_feedback feedback interface of network module
    /// \param scheduler libp2p scheduler
    PeerContext(PeerId peer_id,
                PeerToGraphsyncFeedback &graphsync_feedback,
                PeerToNetworkFeedback &network_feedback,
                libp2p::protocol::Scheduler &scheduler);

    /// Dtor.
    ~PeerContext() override;

    /// Remote peer
    const PeerId peer;

    /// String representation for loggers and debug purposes
    const std::string str;

    /// Stores network address for outbound connections. May be updated.
    /// \param connect_to address, optional
    void setOutboundAddress(
        boost::optional<libp2p::multi::Multiaddress> connect_to);

    /// Returns true if there's no streams to this peer yet
    bool needToConnect();

    /// Internal state, used by Network module
    enum State { can_connect, is_connecting, is_connected, is_closed };

    /// Returns internal state
    State getState() const;

    /// Returns PeerInfo object used to initiate outbound connection
    libp2p::peer::PeerInfo getOutboundPeerInfo() const;

    /// Called on new outbound stream, connected to this peer
    /// \param rstream libp2p stream or error
    void onStreamConnected(outcome::result<StreamPtr> rstream);

    /// Called on new accepted stream from the peer
    /// \param stream libp2p stream
    void onStreamAccepted(StreamPtr stream);

    /// Enqueues new request to the peer. Errors are reported asynchronously
    /// \param request_id request ID
    /// \param request_body request body, raw bytes
    void enqueueRequest(RequestId request_id, SharedData request_body);

    /// Enqueues cancel request to the peer.
    /// \param request_id
    /// \param request_body request body, raw bytes to sent to the network.
    /// Empty body is also allowed, just to cancel the request locally and not
    /// to send anything to the peer
    void cancelRequest(RequestId request_id, SharedData request_body);

    /// Adds data block to response
    /// \param request_id request ID
    /// \param cid CID of the block
    /// \param data data block, raw bytes
    /// \return true if block is added to the response body
    /// and response object itself can be sent
    bool addBlockToResponse(RequestId request_id,
                            const CID &cid,
                            const common::Buffer &data);

    /// Sends response to peer. Data blocks may be added previously
    /// to this response
    /// \param request_id request ID
    /// \param status status code
    /// \param extensions - data for protocol extensions
    void sendResponse(RequestId request_id,
                      ResponseStatusCode status,
                      const std::vector<Extension> &extensions);

    /// Closes all streams to/from this peer
    /// \param status close reason to be forwarded to local request callback,
    /// where RS_REJECTED_LOCALLY indicates that peer was closed by the owning
    /// component, otherwise feedback (peerClosed) is sent asynchronously
    void close(ResponseStatusCode status);

   private:
    /// Per-stream context. Many streams are allowed by design, (i.e. only one
    /// stream sends request to the peer at the moment, but many streams
    /// can be accepted from the peer, we cannot limit other implementation in
    /// this options)
    struct StreamCtx {
      /// Message reader, exists for each connected stream. Purely RAII object
      std::unique_ptr<MessageReader> reader;

      /// Outgoing messages queue
      std::shared_ptr<MessageQueue> queue;

      /// Request IDs from this stream
      std::set<RequestId> remote_request_ids;

      /// Endpoint to responses for this stream
      std::unique_ptr<InboundEndpoint> response_endpoint;

      /// Stream's expire time in Scheduler ticks (milliseconds in real life).
      /// Stream which is inactive during some cleanup period is closed
      /// (activity depends on remote peeris well)
      uint64_t expire_time = 0;
    };

    /// Container for all active streams to/from the peer
    using Streams = std::map<StreamPtr, StreamCtx>;

    /// Feedback from MessageReader objects
    /// \param stream libp2p stream
    /// \param message Graphsync message or read error
    void onReaderEvent(const StreamPtr &stream,
                       outcome::result<Message> message) override;

    /// Feedback from endpoints on async write operations
    /// \param stream libp2p stream
    /// \param message Write result
    void onWriterEvent(const StreamPtr &stream,
                       outcome::result<void> result) override;

    /// Called on new stream. The common part of handling connected
    /// and accepted streams
    /// \param stream libp2p stream
    void onNewStream(StreamPtr stream);

    /// Closes a stream
    /// \param stream libp2p stream
    /// \param status close reason
    void closeStream(StreamPtr stream, ResponseStatusCode status);

    /// Response callback, forwarded to GraphsyncImpl
    /// \param response response wire protocol object
    void onResponse(Message::Response &response);

    /// Request callback, forwarded to GraphsyncImpl
    /// \param stream stream the request came from
    /// \param response response wire protocol object
    void onRequest(const StreamPtr &stream, Message::Request &request);

    /// Creates MessageQueue object in stream context (if doesn't exist yet)
    /// \param stream stream
    /// \param ctx per stream context
    void createMessageQueue(const StreamPtr &stream, StreamCtx &ctx);

    /// Creates responses endpoint object in stream context
    /// (if doesn't exist yet)
    /// \param stream stream
    /// \param ctx per stream context
    void createResponseEndpoint(const StreamPtr &stream, StreamCtx &ctx);

    /// Shifts stream expiration time due to network activity on this stream
    /// \param ctx per stream context
    void shiftExpireTime(StreamCtx &ctx);

    /// Finds context by stream and shifts stream expiration time
    /// \param stream stream
    void shiftExpireTime(const StreamPtr &stream);

    /// Closes all local (i.e. outbound)  requests
    /// \param status status code to be forwarded to callbacks
    void closeLocalRequests(ResponseStatusCode status);

    /// Timer function, performs expired streams cleanup
    void onStreamCleanupTimer();

    /// Finds stream context by remote request id
    /// \param request_id request id made by peer
    /// \return iterator (streams_.end() if not found)
    Streams::iterator findResponseSink(RequestId request_id);

    /// Feedback to GraphsyncImpl module
    PeerToGraphsyncFeedback &graphsync_feedback_;

    /// Feedback to Network module
    PeerToNetworkFeedback &network_feedback_;

    /// Scheduler
    Scheduler &scheduler_;

    /// Outbound address
    boost::optional<libp2p::multi::Multiaddress> connect_to_;

    /// The only one per peer requests sending endpoint
    std::unique_ptr<OutboundEndpoint> requests_endpoint_;

    /// IDs of requests made by this node to the peer
    std::set<RequestId> local_request_ids_;

    /// IDs of requests made by peer mapped to streams they were made through
    std::map<RequestId, StreamPtr> remote_requests_streams_;

    /// Active streams collection
    Streams streams_;

    /// Scheduler's handle, expiration timer
    Scheduler::Handle timer_;

    /// Flag, indicates that peer is closed
    bool closed_ = false;

    /// Response status code stored to be forwarded asynchronously
    /// in the next cycle
    ResponseStatusCode close_status_ = RS_INTERNAL_ERROR;
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_PEER_CONTEXT_HPP
