/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "outbound_endpoint.hpp"

#include <cassert>

#include "message_queue.hpp"

namespace fc::storage::ipfs::graphsync {

  OutboundEndpoint::OutboundEndpoint() : max_pending_bytes_(kMaxPendingBytes) {}

  void OutboundEndpoint::onConnected(std::shared_ptr<MessageQueue> queue) {
    queue_ = std::move(queue);

    assert(queue_);

    is_connecting_ = false;

    for (auto &buf : pending_buffers_) {
      queue_->enqueue(std::move(buf));
    }

    pending_bytes_ = 0;
    pending_buffers_.clear();
  }

  StreamPtr OutboundEndpoint::getStream() const {
    if (queue_) {
      return queue_->getState().stream;
    }
    return StreamPtr{};
  }

  bool OutboundEndpoint::isConnecting() const {
    return is_connecting_;
  }

  bool OutboundEndpoint::isConnected() const {
    return queue_ != nullptr;
  }

  outcome::result<void> OutboundEndpoint::enqueue(SharedData data) {
    if (!data || data->empty()) {
      return outcome::success();
    }

    size_t pending_bytes =
        queue_ ? queue_->getState().pending_bytes : pending_bytes_;

    if (pending_bytes + data->size() > max_pending_bytes_) {
      return Error::WRITE_QUEUE_OVERFLOW;
    }

    if (queue_) {
      queue_->enqueue(std::move(data));
    } else {
      pending_bytes_ += data->size();
      pending_buffers_.emplace_back(std::move(data));
    }

    return outcome::success();
  }

  void OutboundEndpoint::clearPendingMessages() {
    if (queue_) {
      queue_->clear();
    } else {
      pending_buffers_.clear();
      pending_bytes_ = 0;
    }
  }

}  // namespace fc::storage::ipfs::graphsync
