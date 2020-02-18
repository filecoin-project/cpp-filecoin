/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "out_msg_queue.hpp"

#include <cassert>

#include "../errors.hpp"

namespace fc::storage::ipfs::graphsync {

  OutMessageQueue::OutMessageQueue(SessionPtr session, Feedback feedback,
                                   StreamPtr stream, size_t max_pending_bytes)
      : session_(std::move(session)),
        feedback_(std::move(feedback)),
        stream_(std::move(stream)),
        max_pending_bytes_(max_pending_bytes) {
    assert(feedback_);
    assert(stream_);
    assert(max_pending_bytes_ > 0);
  }

  outcome::result<void> OutMessageQueue::write(SharedData msg) {
    if (closed_ || stream_->isClosedForWrite()) {
      return Error::STREAM_NOT_WRITABLE;
    }

    if (msg->empty()) {
      // do nothing on empty messages for consistency
      return outcome::success();
    }

    if (writing_bytes_ > 0) {
      pending_bytes_ += msg->size();
      if (pending_bytes_ > max_pending_bytes_) {
        return Error::MESSAGE_SIZE_OUT_OF_BOUNDS;
      }
      pending_buffers_.emplace_back(std::move(msg));
    } else {
      beginWrite(std::move(msg));
    }

    return outcome::success();
  }

  void OutMessageQueue::onMessageWritten(outcome::result<size_t> res) {
    if (writing_bytes_ == 0) {
      return;
    }

    if (!res) {
      feedback_(session_, res.error());
      return;
    }

    if (writing_bytes_ != res.value()) {
      feedback_(session_, Error::MESSAGE_WRITE_ERROR);
      return;
    }

    writing_bytes_ = 0;

    if (!pending_buffers_.empty()) {
      auto &buffer = pending_buffers_.front();
      pending_bytes_ -= buffer->size();
      beginWrite(std::move(buffer));
      pending_buffers_.pop_front();
    }
  }

  void OutMessageQueue::beginWrite(SharedData buffer) {
    assert(buffer);
    assert(stream_);

    auto data = buffer->data();
    writing_bytes_ = buffer->size();

    // clang-format off
    stream_->write(
        gsl::span(data, writing_bytes_),
        writing_bytes_,
        [wptr{weak_from_this()}, buffer=std::move(buffer)]
            (outcome::result<size_t> result)
        {
          auto self = wptr.lock();
          if (self && !self->closed_) {
            self->onMessageWritten(result);
          }
        }
    );
    // clang-format on
  }

  void OutMessageQueue::close() {
    writing_bytes_ = 0;
    closed_ = true;
    stream_->close([stream{stream_}](outcome::result<void>) {});
    stream_.reset();
  }

}  // namespace fc::storage::ipfs::graphsync
