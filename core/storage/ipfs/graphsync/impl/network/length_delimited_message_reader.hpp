/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_LENGTH_DELIMITED_MESSAGE_READER_HPP
#define CPP_FILECOIN_GRAPHSYNC_LENGTH_DELIMITED_MESSAGE_READER_HPP

#include <functional>

#include <libp2p/common/types.hpp>
#include <libp2p/connection/stream.hpp>

#include "network_fwd.hpp"

// TODO(FIL-144) move this stuff to libp2p

namespace fc::storage::ipfs::graphsync {

  /// Reads varint length delimited messages from connected stream
  class LengthDelimitedMessageReader
      : public std::enable_shared_from_this<LengthDelimitedMessageReader> {
   public:
    LengthDelimitedMessageReader(const LengthDelimitedMessageReader &) = delete;
    LengthDelimitedMessageReader &operator=(
        const LengthDelimitedMessageReader &) = delete;

    /// Feedback interface from reader to its owning object
    using Feedback = std::function<void(const StreamPtr &stream,
                                        outcome::result<ByteArray>)>;

    /// Ctor.
    /// \param feedback Owner's callback
    /// \param max_message_size Message size limit, prevents DoS attacks
    LengthDelimitedMessageReader(Feedback feedback, size_t max_message_size);

    /// Reads an incoming message from stream
    /// \param stream libp2p stream
    /// \return false if stream is not readable, true on success
    bool read(StreamPtr stream);

    /// Closes the reader and stream so that it will ignore further bytes from
    /// wire
    void close();

   private:
    /// Begins async read operation
    void continueReading();

    /// Called when varint prefix is read
    /// \param length Length of message
    void onLengthRead(size_t length);

    /// Called when protocol raw message is read
    /// \param res read result
    void onMessageRead(outcome::result<size_t> res);

    /// libp2p stream
    StreamPtr stream_;

    /// Owner's callback
    Feedback feedback_;

    /// Max message size in bytes
    const size_t max_message_size_;

    /// Internal buffer for async reads
    std::shared_ptr<ByteArray> buffer_;

    /// Internal flag, decouples shared ptr from dependent objects
    bool reading_ = false;
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_LENGH_DELIMITED_MESSAGE_READER_HPP
