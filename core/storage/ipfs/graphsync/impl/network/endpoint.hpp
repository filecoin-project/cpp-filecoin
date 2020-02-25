/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_ENDPOINT_HPP
#define CPP_FILECOIN_GRAPHSYNC_ENDPOINT_HPP

#include "network_fwd.hpp"
#include "storage/ipfs/graphsync/impl/message.hpp"

namespace fc::storage::ipfs::graphsync {

  class EndpointEvents {
   public:
    virtual ~EndpointEvents() = default;

    virtual void onReadEvent(const PeerContextPtr &from,
                             uint64_t endpoint_tag,
                             outcome::result<Message>) = 0;

    virtual void onWriteEvent(const PeerContextPtr &from,
                              uint64_t endpoint_tag,
                              outcome::result<void> result) = 0;
  };

  class LengthDelimitedMessageReader;

  class Endpoint {
   public:
    Endpoint(const Endpoint &) = delete;
    Endpoint &operator=(const Endpoint &) = delete;

    Endpoint(PeerContextPtr peer, uint64_t tag, EndpointEvents &feedback);

    void setStream(StreamPtr stream);

    StreamPtr getStream() const;

    bool is_connected() const;

    /** Reads an incoming message from stream. Returns false if stream is not
     * readable
     */
    bool read();

    /// Enqueues an outgoing message
    void enqueue(SharedData msg);

    void clearOutQueue();

    /// Closes the reader so that it will ignore further bytes from wire
    void close();

   private:
    void onMessageRead(outcome::result<std::vector<uint8_t>> res);

    PeerContextPtr peer_;
    uint64_t tag_;
    EndpointEvents &feedback_;

    std::shared_ptr<LengthDelimitedMessageReader> stream_reader_;

    // TODO(artem) timeouts via scheduler

    StreamPtr stream_;

    /// The queue: streams allow to write messages one by one
    std::deque<SharedData> pending_buffers_;

    /// Number of bytes being awaited in active write operation
    size_t writing_bytes_ = 0;

    /// Max pending bytes
    const size_t max_pending_bytes_;

    /// Current pending bytes
    size_t pending_bytes_ = 0;

    /// Dont send feedback or schedule writes anymore
    bool closed_ = false;
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_ENDPOINT_HPP
