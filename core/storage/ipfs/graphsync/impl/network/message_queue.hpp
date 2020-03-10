/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_MESSAGE_QUEUE_HPP
#define CPP_FILECOIN_GRAPHSYNC_MESSAGE_QUEUE_HPP

#include "network_fwd.hpp"

namespace fc::storage::ipfs::graphsync {

  /// Raw network message queue
  class MessageQueue : public std::enable_shared_from_this<MessageQueue> {
   public:
    /// Feedback: called either on errors or when all buffers written
    using FeedbackFn =
        std::function<void(const StreamPtr &stream, outcome::result<void> res)>;

    /// State of the queue
    struct State {
      /// Libp2p stream
      StreamPtr stream;

      /// Number of bytes being awaited in active write operation
      size_t writing_bytes = 0;

      /// Current pending bytes (enqueued for write)
      size_t pending_bytes = 0;

      /// Total bytes written during lifetime
      size_t total_bytes_written = 0;

      /// True while queue is active
      bool active = false;
    };

    MessageQueue(const MessageQueue &) = delete;
    MessageQueue &operator=(const MessageQueue &) = delete;

    /// Ctor.
    /// \param stream libp2p stream
    /// \param feedback Owner's callback
    MessageQueue(StreamPtr stream, FeedbackFn feedback);

    /// Returns current state
    const State &getState() const;

    /// Enqueues an outgoing message
    void enqueue(SharedData msg);

    /// Clears buffers queue
    void clear();

    /// Closes the queue for further writes
    void close();

   private:
    /// Dequeues message to be written
    void dequeue();

    /// Initiates write operation
    void beginWrite(SharedData buffer);

    /// Async write result callback
    void onMessageWritten(outcome::result<size_t> res);

    /// Owner's callback
    FeedbackFn feedback_;

    /// The queue: streams allow to write messages one by one only
    std::deque<SharedData> pending_buffers_;

    /// Current state of the queue
    State state_;
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_MESSAGE_QUEUE_HPP
