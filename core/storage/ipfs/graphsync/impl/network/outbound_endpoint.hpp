/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "marshalling/response_builder.hpp"
#include "network_fwd.hpp"

namespace fc::storage::ipfs::graphsync {

  class MessageQueue;

  /// Endpoint used to send graphsync msgs to libp2p streams
  class OutboundEndpoint {
   public:
    OutboundEndpoint(const OutboundEndpoint &) = delete;
    OutboundEndpoint &operator=(const OutboundEndpoint &) = delete;

    /// Ctor.
    OutboundEndpoint();

    /// Called by owner (PeerContext) when stream connected
    /// \param queue Message queue, a dependency
    void onConnected(std::shared_ptr<MessageQueue> queue);

    /// Returns stream (streams can be reused)
    StreamPtr getStream() const;

    /// Returns if the endpoint is not already connected
    bool isConnecting() const;

    /// Enqueues an outgoing message
    /// \param msg Raw message body as shared ptr
    /// \return Enqueue result (the queue can be overflown)
    outcome::result<void> enqueue(SharedData msg);

    /// Sends response via message queue
    outcome::result<void> sendResponse(const FullRequestId &id,
                                       const Response &response);

    /// Clears all pending messages
    void clearPendingMessages();

   private:
    /// Adds data block to response. Doesn't send unless sending partial
    /// response is needed
    /// \param cid CID of data block
    /// \param data Raw data
    outcome::result<void> addBlockToResponse(const FullRequestId &request_id,
                                             const CID &cid,
                                             const common::Buffer &data);

    /// Temporary queue for not yet connected endpoint
    std::deque<SharedData> pending_buffers_;

    /// Total bytes in pending buffers
    size_t pending_bytes_ = 0;

    /// Max pending bytes
    const size_t max_pending_bytes_;

    /// Flag, set to true before stream connected
    bool is_connecting_ = true;

    /// Outgoing messages queue
    std::shared_ptr<MessageQueue> queue_;

    /// Wire protocol response messages builder
    ResponseBuilder response_builder_;
  };

}  // namespace fc::storage::ipfs::graphsync
