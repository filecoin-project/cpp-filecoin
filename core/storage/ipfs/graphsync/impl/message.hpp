/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_MESSAGE_HPP
#define CPP_FILECOIN_GRAPHSYNC_MESSAGE_HPP

#include <string>
#include <unordered_set>

#include "types.hpp"

namespace fc::storage::ipfs::graphsync {

  /// Wire protocol message
  struct Message {

    /// Graphsync request
    struct Request {
      /// requester-unique id
      int32_t id = 0;

      /// binary encoded rood CID
      common::Buffer root_cid;

      /// selector string
      common::Buffer selector;

      /// "graphsync/do-not-send-cids" extension
      std::unordered_set<common::Buffer> do_not_send;

      /// "graphsync/response-metadata" extension to be included in response
      bool send_metadata = false;

      /// request priority
      int32_t priority = 1;

      /// cancelling previous known by id
      bool cancel = false;
    };

    /// Response status codes
    enum ResponseStatusCode {
      // info - partial
      RS_REQUEST_ACKNOWLEDGED = 10,  //   Request Acknowledged. Working on it.
      RS_ADDITIONAL_PEERS = 11,      //   Additional Peers. PeerIDs in extra.
      RS_NOT_ENOUGH_GAS = 12,        //   Not enough vespene gas ($)
      RS_OTHER_PROTOCOL = 13,        //   Other Protocol - info in extra.
      RS_PARTIAL_RESPONSE = 14,  //   Partial Response w/metadata, may incl. blocks

      // success - terminal
      RS_FULL_CONTENT = 20,     //   Request Completed, full content.
      RS_PARTIAL_CONTENT = 21,  //   Request Completed, partial content.

      // error - terminal
      RS_REJECTED = 30,        //   Request Rejected. NOT working on it.
      RS_TRY_AGAIN = 31,       //   Request failed, busy, try again later
      RS_REQUEST_FAILED = 32,  //   Request failed, for unknown reason.
      RS_LEGAL_ISSUES = 33,    //   Request failed, for legal reasons.
      RS_NOT_FOUND = 34,       //   Request failed, content not found.
    };

    /// Metadata pairs, is cid present or not
    using ResponseMetadata = std::unordered_map<common::Buffer, bool>;

    /// Graphsync response
    struct Response {
      /// Request id
      int32_t id = 0;

      /// Status code
      ResponseStatusCode status = RS_NOT_FOUND;

      /// Metadata
      ResponseMetadata metadata;
    };

    /// This request list includes *all* requests, replacing outstanding
    /// requests
    bool complete_request_list = false;

    /// The list of requests
    std::vector<std::shared_ptr<Request>> requests;

    /// The list of responses
    std::vector<std::shared_ptr<Response>> responses;

    /// Blocks related to the responses, as cid->data pairs
    std::unordered_map<common::Buffer, common::Buffer> data;
  };
}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_MESSAGE_HPP
