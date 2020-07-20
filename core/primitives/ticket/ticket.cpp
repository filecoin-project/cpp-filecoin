/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/ticket/ticket.hpp"

#include "common/le_encoder.hpp"

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

  int compare(const Ticket &lhs, const Ticket &rhs) {
    return memcmp(lhs.bytes.data(), rhs.bytes.data(), lhs.bytes.size());
  }

  bool operator==(const Ticket &lhs, const Ticket &rhs) {
    return compare(lhs, rhs) == 0;
  }

  bool operator<(const Ticket &lhs, const Ticket &rhs) {
    return compare(lhs, rhs) < 0;
  }

}  // namespace fc::primitives::ticket
