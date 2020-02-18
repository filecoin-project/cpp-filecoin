/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphsync_message_reader.hpp"

#include <cassert>

namespace fc::storage::ipfs::graphsync {

  GraphsyncMessageReader::GraphsyncMessageReader(SessionPtr session,
                                                 size_t max_message_size)
      : session_(std::move(session)) {
    assert(session);
    assert(max_message_size > 0);

    stream_reader_ = std::make_shared<LengthDelimitedMessageReader>(
        [this](outcome::result<ByteArray> res) {
          onMessageRead(std::move(res));
        },
        max_message_size);
  }

  bool GraphsyncMessageReader::read(StreamPtr stream, Feedback feedback) {
    assert(feedback);
    feedback_ = std::move(feedback);
    return stream_reader_->read(std::move(stream));
  }

  void GraphsyncMessageReader::onMessageRead(outcome::result<ByteArray> res) {
    assert(feedback_);

    if (!res) {
      return feedback_(session_, res.error());
    }

    auto msg_res = parseMessage(res.value());
    if (!msg_res) {
      return feedback_(session_, msg_res.error());
    }

    feedback_(session_, std::move(msg_res.value()));
  }

  void GraphsyncMessageReader::close() {
    stream_reader_->close();
  }

}  // namespace fc::storage::ipfs::graphsync
