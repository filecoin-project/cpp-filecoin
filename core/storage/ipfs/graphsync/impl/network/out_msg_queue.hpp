/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_OUT_MSG_QUEUE_HPP
#define CPP_FILECOIN_OUT_MSG_QUEUE_HPP

#include <deque>
#include <functional>

#include <libp2p/connection/stream.hpp>
#include "../types.hpp"

namespace fc::storage::ipfs::graphsync {

  /// Queues and writes length delimited messages to connected stream
  class OutMessageQueue : public std::enable_shared_from_this<OutMessageQueue> {
   public:
    /// Feedback interface from writer to its owning object
    using Feedback = std::function<void(const SessionPtr &from,
                                        outcome::result<void> error)>;

    OutMessageQueue(SessionPtr session, Feedback feedback, StreamPtr stream,
                    size_t max_pending_bytes);

    /// Writes an outgoing message to stream. Returns error if closed
    outcome::result<void> write(SharedData msg);

    /// Closes writer and discards all outgoing messages
    void close();

   private:
    void onMessageWritten(outcome::result<size_t> res);
    void beginWrite(SharedData buffer);

    SessionPtr session_;
    Feedback feedback_;
    StreamPtr stream_;

    /// The queue: streams allow to write messages one by one
    std::deque<SharedData> pending_buffers_;

    /// Number of bytes being awaited in active wrote operation
    size_t writing_bytes_ = 0;

    /// Max pending bytes
    const size_t max_pending_bytes_;

    /// Current pending bytes
    size_t pending_bytes_ = 0;

    /// Dont send feedback or schedule writes anymore
    bool closed_ = false;

    // TODO(artem): timeouts
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_OUT_MSG_QUEUE_HPP
