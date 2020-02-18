/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_MESSAGE_PARSER_HPP
#define CPP_FILECOIN_GRAPHSYNC_MESSAGE_PARSER_HPP

#include "../types.hpp"
#include "../message.hpp"

namespace fc::storage::ipfs::graphsync {

  /// Parses protobuf message received from wire
  outcome::result<Message> parseMessage(gsl::span<const uint8_t> bytes);

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_MESSAGE_PARSER_HPP
