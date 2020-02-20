/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_LENGTH_DELIMITED_MESSAGE_READER_HPP
#define CPP_FILECOIN_GRAPHSYNC_LENGTH_DELIMITED_MESSAGE_READER_HPP

#include <functional>

#include <libp2p/common/types.hpp>
#include <libp2p/connection/stream.hpp>

#include "../errors.hpp"
#include "../types.hpp"

namespace fc::storage::ipfs::graphsync {

  /// Reads varint length delimited messages from connected stream
  class LengthDelimitedMessageReader
      : public std::enable_shared_from_this<LengthDelimitedMessageReader> {
   public:
    LengthDelimitedMessageReader(const LengthDelimitedMessageReader &) = delete;
    LengthDelimitedMessageReader &operator=(
        const LengthDelimitedMessageReader &) = delete;

    /// Feedback interface from reader to its owning object
    using Feedback = std::function<void(outcome::result<ByteArray>)>;

    LengthDelimitedMessageReader(Feedback feedback, size_t max_message_size);

    /**
     * Reads an incoming message from stream. Returns false if stream is not
     * readable
     */
    bool read(StreamPtr stream);

    /// Closes the reader and stream so that it will ignore further bytes from
    /// wire
    void close();

   private:
    void continueReading();
    void onLengthRead(size_t length);
    void onMessageRead(outcome::result<size_t> res);

    StreamPtr stream_;
    Feedback feedback_;
    const size_t max_message_size_;
    std::shared_ptr<ByteArray> buffer_;
    bool reading_ = false;
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_LENGH_DELIMITED_MESSAGE_READER_HPP
