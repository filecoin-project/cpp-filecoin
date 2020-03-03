/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_OUTBOUND_ENDPOINT_HPP
#define CPP_FILECOIN_GRAPHSYNC_OUTBOUND_ENDPOINT_HPP

#include "network_fwd.hpp"

namespace fc::storage::ipfs::graphsync {

  class MessageQueue;

  class OutboundEndpoint {
   public:
    OutboundEndpoint(const OutboundEndpoint &) = delete;
    OutboundEndpoint &operator=(const OutboundEndpoint &) = delete;

    OutboundEndpoint();

    void onConnected(std::shared_ptr<MessageQueue> queue);

    StreamPtr getStream() const;

    bool isConnecting() const;

    bool isConnected() const;

    /// Enqueues an outgoing message
    outcome::result<void> enqueue(SharedData msg);

    void clearPendingMessages();

   private:
    /// Temporary queue for not yet connected endpoint
    std::deque<SharedData> pending_buffers_;

    /// Total bytes in pending buffers
    size_t pending_bytes_ = 0;

    /// Max pending bytes
    const size_t max_pending_bytes_;

    bool is_connecting_ = true;

    std::shared_ptr<MessageQueue> queue_;
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_OUTBOUND_ENDPOINT_HPP
