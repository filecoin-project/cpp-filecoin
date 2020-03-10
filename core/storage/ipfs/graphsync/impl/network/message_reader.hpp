/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_MESSAGE_READER_HPP
#define CPP_FILECOIN_GRAPHSYNC_MESSAGE_READER_HPP

#include "network_fwd.hpp"

namespace fc::storage::ipfs::graphsync {

  class LengthDelimitedMessageReader;

  /// Per-stream message reader, graphsync specific
  class MessageReader {
   public:
    /// Ctor.
    /// \param stream libp2p stream
    /// \param feedback Owner's feedback interface
    MessageReader(StreamPtr stream,
                  EndpointToPeerFeedback &feedback);

    ~MessageReader();

   private:
    /// Callback for async length delimited read operations
    /// \param stream
    /// \param res
    void onMessageRead(const StreamPtr &stream, outcome::result<ByteArray> res);

    /// Owner's feedback interface
    EndpointToPeerFeedback &feedback_;

    /// Stream reader, not graphsync specific
    std::shared_ptr<LengthDelimitedMessageReader> stream_reader_;
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_MESSAGE_READER_HPP
