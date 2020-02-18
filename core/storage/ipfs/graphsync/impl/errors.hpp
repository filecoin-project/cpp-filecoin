/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_ERRORS_HPP
#define CPP_FILECOIN_GRAPHSYNC_ERRORS_HPP

#include <libp2p/outcome/outcome.hpp>

namespace fc::storage::ipfs::graphsync {

  //TODO move to fc repo
  namespace outcome = libp2p::outcome;

  /// Graphsync error codes
  enum class Error {
    MESSAGE_SIZE_OUT_OF_BOUNDS,
    MESSAGE_PARSE_ERROR,
    MESSAGE_VALIDATION_FAILED,
    MESSAGE_SERIALIZE_ERROR,
    STREAM_NOT_READABLE,
    MESSAGE_READ_ERROR,
    STREAM_NOT_WRITABLE,
    WRITE_QUEUE_OVERFLOW,
    MESSAGE_WRITE_ERROR,
  };

}  // namespace fc::storage::ipfs::graphsync

OUTCOME_HPP_DECLARE_ERROR(fc::storage::ipfs::graphsync, Error);


#endif  // CPP_FILECOIN_GRAPHSYNC_ERRORS_HPP
