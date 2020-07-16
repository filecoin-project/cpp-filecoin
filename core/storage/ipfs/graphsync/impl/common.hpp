/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_COMMON_HPP
#define CPP_FILECOIN_GRAPHSYNC_COMMON_HPP

#include "common/logger.hpp"
#include "storage/ipfs/graphsync/graphsync.hpp"

namespace fc::storage::ipfs::graphsync {

  /// Graphsync internal error codes
  enum class Error {
    kMessageSizeOutOfBounds = 1,
    kMessageParseError,
    kMessageValidationFailed,
    kMessageSerializeError,
    kStreamNotReadable,
    kMessageReadError,
    kStreamNotWritable,
    kWriteQueueOverflow,
    kMessageWriteError,
  };

  /// Request ID defined as int32 by graphsync protocol, unfortunately
  using RequestId = int32_t;

  /// Reusing ByteArray from libp2p
  using libp2p::common::ByteArray;

  /// Using shared ptrs for outgoing raw messages
  using SharedData = std::shared_ptr<const ByteArray>;

  /// Using libp2p and its peer Ids
  using libp2p::peer::PeerId;

  /// Returns shared logger for graphsync modules
  common::Logger logger();
}  // namespace fc::storage::ipfs::graphsync

OUTCOME_HPP_DECLARE_ERROR(fc::storage::ipfs::graphsync, Error);

#endif  // CPP_FILECOIN_GRAPHSYNC_COMMON_HPP
