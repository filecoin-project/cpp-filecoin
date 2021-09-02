/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/ipfs/graphsync/impl/network/marshalling/message.hpp"

namespace fc::storage::ipfs::graphsync {

  /// Parses protobuf message received from wire
  /// \param bytes Raw bytes of received message, without length prefix
  /// \return Message or error
  outcome::result<Message> parseMessage(gsl::span<const uint8_t> bytes);

}  // namespace fc::storage::ipfs::graphsync
