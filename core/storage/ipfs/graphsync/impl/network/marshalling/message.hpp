/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_MESSAGE_HPP
#define CPP_FILECOIN_GRAPHSYNC_MESSAGE_HPP

#include "storage/ipfs/graphsync/impl/common.hpp"

namespace fc::storage::ipfs::graphsync {

  /// Wire protocol message
  struct Message {
    /// Graphsync request
    struct Request {
      /// Requester-unique id, int32_t
      RequestId id = 0;

      /// Root CID
      CID root_cid;

      /// Selector string
      common::Buffer selector;

      /// "graphsync/do-not-send-cids" extension
      std::vector<CID> do_not_send;

      /// "graphsync/response-metadata" extension whether metadata is to be
      /// included in response
      bool send_metadata = false;

      /// request priority: not used at the moment
      int32_t priority = 1;

      /// flag which cancels previous request with the same id
      bool cancel = false;
    };

    /// Graphsync response
    struct Response {
      /// Request id
      RequestId id = 0;

      /// Status code
      ResponseStatusCode status = RS_NOT_FOUND;

      /// Metadata
      ResponseMetadata metadata;
    };

    /// This request list includes *all* requests from the peer, replacing
    /// existing outstanding requests
    bool complete_request_list = false;

    /// The list of requests
    std::vector<Request> requests;

    /// The list of responses
    std::vector<Response> responses;

    /// Blocks related to the responses, as cid->data pairs
    std::vector<std::pair<CID, common::Buffer>> data;
  };
}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_MESSAGE_HPP
