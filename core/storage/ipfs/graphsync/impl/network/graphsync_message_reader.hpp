/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_MESSAGE_READER_HPP
#define CPP_FILECOIN_GRAPHSYNC_MESSAGE_READER_HPP

#include "../marshalling/message_parser.hpp"
#include "length_delimited_message_reader.hpp"

namespace fc::storage::ipfs::graphsync {

  /// Reads graphsync messages from connected stream
  class GraphsyncMessageReader {
   public:
    GraphsyncMessageReader(const GraphsyncMessageReader &) = delete;
    GraphsyncMessageReader &operator=(const GraphsyncMessageReader &) = delete;

    /// Feedback interface from reader to its owning object
    using Feedback =
        std::function<void(const SessionPtr &from, outcome::result<Message>)>;

    GraphsyncMessageReader(SessionPtr session, size_t max_message_size);

    /** Reads an incoming message from stream. Returns false if stream is not
     * readable
     */
    bool read(StreamPtr stream, Feedback feedback);

    /// Closes the reader so that it will ignore further bytes from wire
    void close();

   private:
    void onMessageRead(outcome::result<ByteArray> res);

    SessionPtr session_;
    Feedback feedback_;
    std::shared_ptr<LengthDelimitedMessageReader> stream_reader_;
    // TODO(artem) timeouts via scheduler
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_MESSAGE_READER_HPP
