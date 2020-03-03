/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_INBOUND_ENDPOINT_HPP
#define CPP_FILECOIN_GRAPHSYNC_INBOUND_ENDPOINT_HPP

#include "marshalling/response_builder.hpp"
#include "network_fwd.hpp"

namespace fc::storage::ipfs::graphsync {

  class MessageQueue;

  class InboundEndpoint {
   public:
    InboundEndpoint(const InboundEndpoint &) = delete;
    InboundEndpoint &operator=(const InboundEndpoint &) = delete;

    explicit InboundEndpoint(std::shared_ptr<MessageQueue> queue);

    outcome::result<void> addBlockToResponse(int request_id,
                                             const CID &cid,
                                             const common::Buffer &data);

    outcome::result<void> sendResponse(int request_id,
                                       ResponseStatusCode status,
                                       const ResponseMetadata &metadata);

   private:
    outcome::result<void> sendPartialResponse(int request_id);

    /// Max pending bytes
    const size_t max_pending_bytes_;

    std::shared_ptr<MessageQueue> queue_;

    ResponseBuilder response_builder_;
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_INBOUND_ENDPOINT_HPP
