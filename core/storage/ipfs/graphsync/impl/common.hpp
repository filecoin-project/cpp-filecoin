/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_COMMON_HPP
#define CPP_FILECOIN_GRAPHSYNC_COMMON_HPP

#include "storage/ipfs/graphsync/graphsync.hpp"

namespace fc::storage::ipfs::graphsync {

  /// Graphsync error codes
  enum class Error {
    MESSAGE_SIZE_OUT_OF_BOUNDS = 1,
    MESSAGE_PARSE_ERROR,
    MESSAGE_VALIDATION_FAILED,
    MESSAGE_SERIALIZE_ERROR,
    STREAM_NOT_READABLE,
    MESSAGE_READ_ERROR,
    STREAM_NOT_WRITABLE,
    WRITE_QUEUE_OVERFLOW,
    MESSAGE_WRITE_ERROR,
  };

  using libp2p::common::ByteArray;
  using SharedData = std::shared_ptr<const ByteArray>;
  using libp2p::peer::PeerId;

  constexpr std::string_view kResponseMetadata = "graphsync/response-metadata";
  constexpr std::string_view kDontSendCids = "graphsync/do-not-send-cids";
  constexpr std::string_view kLink = "link";
  constexpr std::string_view kBlockPresent = "blockPresent";

 // TODO - extract error category
 //  ResponseStatusCode errorToStatusCode(outcome::result<void> error);

}  // namespace fc::storage::ipfs::graphsync

OUTCOME_HPP_DECLARE_ERROR(fc::storage::ipfs::graphsync, Error);

#endif  // CPP_FILECOIN_GRAPHSYNC_COMMON_HPP
