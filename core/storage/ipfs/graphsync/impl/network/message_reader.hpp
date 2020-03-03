/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_MESSAGE_READER_HPP
#define CPP_FILECOIN_GRAPHSYNC_MESSAGE_READER_HPP

#include "network_fwd.hpp"

namespace fc::storage::ipfs::graphsync {

  class LengthDelimitedMessageReader;

  /// Per-stream message reader
  class MessageReader {
   public:
    MessageReader(StreamPtr stream,
                  EndpointToPeerFeedback &feedback);

    ~MessageReader();

   private:
    void onMessageRead(const StreamPtr &stream, outcome::result<ByteArray> res);

    PeerContextPtr peer_;
    EndpointToPeerFeedback &feedback_;

    /// Stream reader
    std::shared_ptr<LengthDelimitedMessageReader> stream_reader_;
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_MESSAGE_READER_HPP
