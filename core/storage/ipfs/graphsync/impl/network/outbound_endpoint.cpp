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

  outcome::result<void> OutboundEndpoint::enqueue(SharedData data) {
    if (!data || data->empty()) {
      return outcome::success();
    }

    size_t pending_bytes =
        queue_ ? queue_->getState().pending_bytes : pending_bytes_;

    if (pending_bytes + data->size() > max_pending_bytes_) {
      logger()->warn("OutboundEndpoint::enqueue overflow {}/{}",
                     pending_bytes + data->size(),
                     max_pending_bytes_);
    }

    if (queue_) {
      queue_->enqueue(std::move(data));
    } else {
      pending_bytes_ += data->size();
      pending_buffers_.emplace_back(std::move(data));
    }

    return outcome::success();
  }

  outcome::result<void> OutboundEndpoint::sendResponse(
      const FullRequestId &id, const Response &response) {
    for (const auto &block : response.data) {
      OUTCOME_TRY(addBlockToResponse(id, block.cid, block.content));
    }

    response_builder_.addResponse(id.id, response.status, response.extensions);
    auto res = response_builder_.serialize();
    response_builder_.clear();
    if (!res) {
      return res.error();
    }

    logger()->debug("enqueueing response, size={}", res.value()->size());

    return enqueue(std::move(res.value()));
  }

  outcome::result<void> OutboundEndpoint::addBlockToResponse(
      const FullRequestId &request_id, const CID &cid, const Bytes &data) {
    auto serialized_size = response_builder_.getSerializedSize();

    if (serialized_size + data.size() > kMaxMessageSize) {
      static const Response partial{RS_PARTIAL_RESPONSE, {}, {}};
      OUTCOME_TRY(sendResponse(request_id, partial));
    }

    response_builder_.addDataBlock(cid, data);
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

  bool OutboundEndpoint::empty() const {
    if (queue_) {
      auto state{queue_->getState()};
      return state.pending_bytes == 0 && state.writing_bytes == 0;
    }
    return pending_bytes_ == 0;
  }
}  // namespace fc::storage::ipfs::graphsync
