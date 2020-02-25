/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_REQUEST_BUILDER_HPP
#define CPP_FILECOIN_GRAPHSYNC_REQUEST_BUILDER_HPP

#include "message_builder.hpp"

namespace fc::storage::ipfs::graphsync {

  /// Collects request entries and serializes them to wire protocol
  class RequestBuilder : public MessageBuilder {
   public:
    void addRequest(int request_id,
                    const CID &root_cid,
                    gsl::span<const uint8_t> selector,
                    bool need_metadata,
                    const std::vector<CID> &dont_send_cids);

    void addCancelRequest(int request_id);

    void setCompleteRequestList();
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_REQUEST_BUILDER_HPP
