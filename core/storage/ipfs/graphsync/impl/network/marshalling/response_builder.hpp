/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_RESPONSE_BUILDER_HPP
#define CPP_FILECOIN_GRAPHSYNC_RESPONSE_BUILDER_HPP

#include "message_builder.hpp"

namespace fc::storage::ipfs::graphsync {

  /// Collects response entries and serializes them to wire protocol
  class ResponseBuilder : public MessageBuilder {
   public:
    /// Adds response to protobuf message
    /// \param request_id id of request
    /// \param status status code
    /// \param extensions - data for protocol extensions
    void addResponse(RequestId request_id,
                     ResponseStatusCode status,
                     const std::vector<Extension> &extensions);

    /// Adds data block to protobuf message
    /// \param cid CID of data block
    /// \param data Raw data
    void addDataBlock(const CID &cid, const common::Buffer &data);
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_RESPONSE_BUILDER_HPP
