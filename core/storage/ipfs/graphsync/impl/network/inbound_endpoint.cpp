/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "inbound_endpoint.hpp"

#include "message_queue.hpp"
#include "peer_context.hpp"

namespace fc::storage::ipfs::graphsync {

  InboundEndpoint::InboundEndpoint(std::shared_ptr<MessageQueue> queue)
      : max_pending_bytes_(kMaxPendingBytes), queue_(std::move(queue)) {
    assert(queue_);
  }

  outcome::result<void> InboundEndpoint::addBlockToResponse(
      int request_id, const CID &cid, const common::Buffer &data) {
    auto serialized_size = response_builder_.getSerializedSize();

    if (queue_->getState().pending_bytes + serialized_size + data.size()
        > max_pending_bytes_) {
      return Error::WRITE_QUEUE_OVERFLOW;
    }

    if (serialized_size + data.size() > kMaxMessageSize) {
      auto res = sendPartialResponse(request_id);
      if (!res) {
        return res;
      }
    }

    response_builder_.addDataBlock(cid, data);
    return outcome::success();
  }

  outcome::result<void> InboundEndpoint::sendResponse(
      int request_id,
      ResponseStatusCode status,
      const ResponseMetadata &metadata) {
    response_builder_.addResponse(request_id, status, metadata);
    auto res = response_builder_.serialize();
    response_builder_.clear();
    if (!res) {
      return res.error();
    }

    if (queue_->getState().pending_bytes > max_pending_bytes_) {
      return Error::WRITE_QUEUE_OVERFLOW;
    }

    queue_->enqueue(std::move(res.value()));
    return outcome::success();
  }

  outcome::result<void> InboundEndpoint::sendPartialResponse(int request_id) {
    static const ResponseMetadata dummy_metadata;
    return sendResponse(request_id, RS_PARTIAL_CONTENT, dummy_metadata);
  }

}  // namespace fc::storage::ipfs::graphsync
