/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "length_delimited_message_reader.hpp"

#include <cassert>

#include <libp2p/basic/varint_reader.hpp>

namespace fc::storage::ipfs::graphsync {

  LengthDelimitedMessageReader::LengthDelimitedMessageReader(
      Feedback feedback, size_t max_message_size)
      : feedback_(std::move(feedback)),
        max_message_size_(max_message_size),
        buffer_(std::make_shared<std::vector<uint8_t>>()) {
    assert(feedback_);
  }

  bool LengthDelimitedMessageReader::read(StreamPtr stream) {
    if (stream->isClosedForRead()) {
      return false;
    }

    if (stream_ != stream) {
      close();
      stream_ = std::move(stream);
    }

    if (stream_) {
      continueReading();
    }

    return true;
  }

  void LengthDelimitedMessageReader::continueReading() {
    assert(stream_);

    if (reading_) {
      return;
    }

    // clang-format off
    libp2p::basic::VarintReader::readVarint(
        stream_,
        [wptr{weak_from_this()}]
            (boost::optional<libp2p::multi::UVarint> varint_opt) {
          auto self = wptr.lock();
          if (self && self->reading_) {
            size_t length = varint_opt ? varint_opt->toUInt64() : 0;
            self->onLengthRead(length);
          }
        }
    );
    // clang-format on

    reading_ = true;
  }

  void LengthDelimitedMessageReader::onLengthRead(size_t length) {
    if (!reading_) {
      return;
    }

    if (length == 0) {
      reading_ = false;
      return feedback_(stream_, Error::STREAM_NOT_READABLE);
    }

    if (length > max_message_size_) {
      reading_ = false;
      return feedback_(stream_, Error::MESSAGE_SIZE_OUT_OF_BOUNDS);
    }

    buffer_->resize(length);

    // clang-format off
    stream_->read(
        gsl::span(buffer_->data(), length),
        length,
        [wptr{weak_from_this()}, buffer{buffer_}](outcome::result<size_t> res) {
          auto self = wptr.lock();
          if (self && self->reading_) {
            self->onMessageRead(res);
          }
        }
    );
    // clang-format on
  }

  void LengthDelimitedMessageReader::onMessageRead(
      outcome::result<size_t> res) {
    if (!reading_) {
      return;
    }

    reading_ = false;

    if (!res) {
      return feedback_(stream_, res.error());
    }

    if (buffer_->size() != res.value()) {
      return feedback_(stream_, Error::MESSAGE_READ_ERROR);
    }

    feedback_(stream_, std::move(*buffer_));

    // if owner called close() during feedback then the stream is now reset
    // and no further reading is needed
    if (stream_) {
      continueReading();
    }
  }

  void LengthDelimitedMessageReader::close() {
    if (!stream_) {
      return;
    }
    reading_ = false;
    if (!stream_->isClosedForRead() && stream_.use_count() == 1) {
      stream_->close([stream{stream_}](outcome::result<void>) {});
    }
    stream_.reset();
  }

}  // namespace fc::storage::ipfs::graphsync
