/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/primitives/ticket/ticket.hpp"

#include "filecoin/common/le_encoder.hpp"

namespace fc::primitives::ticket {
  using crypto::randomness::Randomness;

  libp2p::outcome::result<Randomness> drawRandomness(const Ticket &ticket,
                                                     int64_t round) {
    common::Buffer buffer{};
    buffer.put(ticket.bytes);
    common::encodeLebInteger(round, buffer);
    auto &&hash = libp2p::crypto::sha256(buffer);

    return Randomness::fromSpan(hash);
  }

  bool operator==(const Ticket &lhs, const Ticket &rhs) {
    return lhs.bytes == rhs.bytes;
  }

  bool operator<(const Ticket &lhs, const Ticket &rhs) {
    return lhs.bytes < rhs.bytes;
  }

}  // namespace fc::primitives::ticket
