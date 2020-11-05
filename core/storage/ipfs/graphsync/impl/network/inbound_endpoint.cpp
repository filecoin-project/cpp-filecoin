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

  outcome::result<void> InboundEndpoint::sendResponse(
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

    if (queue_->getState().pending_bytes > max_pending_bytes_) {
      return Error::kWriteQueueOverflow;
    }

    queue_->enqueue(std::move(res.value()));
    return outcome::success();
  }

  outcome::result<void> InboundEndpoint::addBlockToResponse(
      const FullRequestId &request_id,
      const CID &cid,
      const common::Buffer &data) {
    auto serialized_size = response_builder_.getSerializedSize();

    if (queue_->getState().pending_bytes + serialized_size + data.size()
        > max_pending_bytes_) {
      return Error::kWriteQueueOverflow;
    }

    if (serialized_size + data.size() > kMaxMessageSize) {
      static const Response partial{RS_PARTIAL_CONTENT, {}, {}};
      OUTCOME_TRY(sendResponse(request_id, partial));
    }

    response_builder_.addDataBlock(cid, data);
    return outcome::success();
  }

}  // namespace fc::storage::ipfs::graphsync
