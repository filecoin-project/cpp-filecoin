/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_RESPONSE_BUILDER_HPP
#define CPP_FILECOIN_GRAPHSYNC_RESPONSE_BUILDER_HPP

#include <libp2p/multi/content_identifier.hpp>

#include "message_builder.hpp"

namespace fc::storage::ipfs::graphsync {

  /// Collects response entries and serializes them to wire protocol
  class ResponseBuilder : public MessageBuilder {
   public:

    void addResponse(const Message::Response &resp);

    void addDataBlock(const libp2p::multi::ContentIdentifier &cid,
                      const ByteArray &data);
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_RESPONSE_BUILDER_HPP
