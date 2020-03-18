/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_OUTBOUND_ENDPOINT_HPP
#define CPP_FILECOIN_GRAPHSYNC_OUTBOUND_ENDPOINT_HPP

#include "network_fwd.hpp"

namespace fc::storage::ipfs::graphsync {

  class MessageQueue;

  /// Endpoint used to send request to libp2p streams
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

    /// Resurns if the endpoint is not already connected
    bool isConnecting() const;

    /// Resurns if the endpoint is connected
    bool isConnected() const;

    /// Enqueues an outgoing message
    /// \param msg Raw message body as shared ptr
    /// \return Enqueue result (the queue can be overflown)
    outcome::result<void> enqueue(SharedData msg);

    /// Clears all pending messages
    void clearPendingMessages();

   private:
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
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_OUTBOUND_ENDPOINT_HPP
