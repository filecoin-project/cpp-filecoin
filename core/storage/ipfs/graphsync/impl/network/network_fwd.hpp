/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_NETWORK_FWD_HPP
#define CPP_FILECOIN_GRAPHSYNC_NETWORK_FWD_HPP

#include <libp2p/protocol/common/scheduler.hpp>
#include "marshalling/message.hpp"

namespace libp2p::connection {
  // libp2p stream forward decl
  class Stream;
}  // namespace libp2p::connection

namespace fc::storage::ipfs::graphsync {

  /// Libp2p stream, used by shared ptr
  using StreamPtr = std::shared_ptr<libp2p::connection::Stream>;

  /// Libp2p scheduler
  using Scheduler = libp2p::protocol::Scheduler;

  /// PeerContext used by Network component to communicate with any peer
  class PeerContext;

  /// PeerContext used by shard ptr only
  using PeerContextPtr = std::shared_ptr<PeerContext>;

  /// PeerContext->GraphsyncImpl feedback interface
  class PeerToGraphsyncFeedback {
   public:
    virtual ~PeerToGraphsyncFeedback() = default;

    /// Called on new block from the network
    /// \param from originating peer ID
    /// \param cid root CID
    /// \param data block data, raw bytes
    virtual void onBlock(const PeerId &from, CID cid, common::Buffer data) = 0;

    /// Called on new request from the network
    /// \param from originating peer ID
    /// \param request request received from wire
    virtual void onRemoteRequest(const PeerId &from,
                                 Message::Request request) = 0;

    /// Called when response to a local request is received
    /// \param peer peer ID of the response
    /// \param request_id request ID
    /// \param status status code
    /// \param extensions - data for protocol extensions
    virtual void onResponse(const PeerId &peer,
                            RequestId request_id,
                            ResponseStatusCode status,
                            std::vector<Extension> extensions) = 0;
  };

  /// PeerContext->Network feedback interface
  class PeerToNetworkFeedback {
   public:
    virtual ~PeerToNetworkFeedback() = default;

    /// Called when peer is closed
    /// \param peer peer ID
    /// \param status close reason as ResponseStatusCode
    virtual void peerClosed(const PeerId &peer, ResponseStatusCode status) = 0;
  };

  /// Request/response endpoints to PeerContext feedback interface
  class EndpointToPeerFeedback {
   public:
    virtual ~EndpointToPeerFeedback() = default;

    /// Called on graphsync message received
    /// \param stream libp2p stream
    /// \param message Graphsync message or read error
    virtual void onReaderEvent(const StreamPtr &stream,
                               outcome::result<Message> message) = 0;

    /// Called on message is written asynchronously
    /// \param stream libp2p stream
    /// \param message Write result
    virtual void onWriterEvent(const StreamPtr &stream,
                               outcome::result<void> result) = 0;
  };

  /// Operators needed to find a PeerContext in a set by peer id
  bool operator<(const PeerContextPtr &ctx, const PeerId &peer);
  bool operator<(const PeerId &peer, const PeerContextPtr &ctx);
  bool operator<(const PeerContextPtr &a, const PeerContextPtr &b);

  /// Protocol version
  constexpr std::string_view kProtocolVersion = "/ipfs/graphsync/1.0.0";

  /// Max byte size of individual message
  constexpr size_t kMaxMessageSize = 16 * 1024 * 1024;

  /// Max byte size of pending message queue
  constexpr size_t kMaxPendingBytes = 64 * 1024 * 1024;

  /// Cleanup delay for PeerContext, msec
  constexpr unsigned kPeerCloseDelayMsec = 30000;

  /// Cleanup delay for stream, msec
  constexpr unsigned kStreamCloseDelayMsec = 60000;

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_NETWORK_FWD_HPP
