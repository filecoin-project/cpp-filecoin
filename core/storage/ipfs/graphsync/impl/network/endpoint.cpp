/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "endpoint.hpp"

#include <cassert>

#include "length_delimited_message_reader.hpp"
#include "marshalling/message_parser.hpp"

namespace fc::storage::ipfs::graphsync {

  Endpoint::Endpoint(PeerContextPtr peer,
                     uint64_t tag,
                     EndpointEvents &feedback)
      : peer_(std::move(peer)),
        tag_(tag),
        feedback_(feedback),
        max_pending_bytes_(kMaxPendingBytes) {}

  Endpoint::~Endpoint() {
    close();
  }

  void Endpoint::setStream(StreamPtr stream) {
    stream_ = std::move(stream);
    if (stream_) {
      closed_ = false;
      dequeue();
    }
  }

  StreamPtr Endpoint::getStream() const {
    return stream_;
  }

  bool Endpoint::is_connected() const {
    return !closed_ && !stream_->isClosed();
  }

  bool Endpoint::read() {
    if (!is_connected()) {
      return false;
    }
    if (!stream_reader_) {
      stream_reader_ = std::make_shared<LengthDelimitedMessageReader>(
          [this](outcome::result<ByteArray> res) {
            onMessageRead(std::move(res));
          },
          kMaxMessageSize);
    }
    return stream_reader_->read(stream_);
  }

  outcome::result<void> Endpoint::enqueue(SharedData data) {
    if (!data || data->empty()) {
      // do nothing on empty messages for consistency
      return outcome::success();
    }

    if (writing_bytes_ > 0 || closed_) {
      pending_bytes_ += data->size();
      if (pending_bytes_ > max_pending_bytes_) {
        return Error::MESSAGE_SIZE_OUT_OF_BOUNDS;
      }
      pending_buffers_.emplace_back(std::move(data));
    } else {
      beginWrite(std::move(data));
    }

    return outcome::success();
  }

  void Endpoint::clearOutQueue() {
    pending_buffers_.clear();
    pending_bytes_ = 0;
  }

  void Endpoint::close() {
    closed_ = true;
    writing_bytes_ = 0;
    if (stream_reader_) stream_reader_->close();
    clearOutQueue();
    stream_.reset();
  }

  void Endpoint::dequeue() {
    if (!pending_buffers_.empty()) {
      auto &buffer = pending_buffers_.front();
      pending_bytes_ -= buffer->size();
      beginWrite(std::move(buffer));
      pending_buffers_.pop_front();
    }
  }

  void Endpoint::beginWrite(SharedData buffer) {
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

  void Endpoint::onMessageWritten(outcome::result<size_t> res) {
    if (writing_bytes_ == 0) {
      // dont need the result anymore
      return;
    }

    if (!res) {
      feedback_.onWriteEvent(peer_, tag_, res.error());
      return;
    }

    if (writing_bytes_ != res.value()) {
      feedback_.onWriteEvent(peer_, tag_, Error::MESSAGE_WRITE_ERROR);
      return;
    }

    writing_bytes_ = 0;

    dequeue();
  }

  void Endpoint::onMessageRead(outcome::result<ByteArray> res) {
    if (closed_) {
      return;
    }

    if (!res) {
      return feedback_.onReadEvent(peer_, tag_, res.error());
    }

    auto msg_res = parseMessage(res.value());
    if (!msg_res) {
      return feedback_.onReadEvent(peer_, tag_, msg_res.error());
    }

    feedback_.onReadEvent(peer_, tag_, std::move(msg_res.value()));
  }

}  // namespace fc::storage::ipfs::graphsync
