/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/ticket/ticket.hpp"

#include "common/le_encoder.hpp"

namespace fc::primitives::ticket {
  using crypto::randomness::Randomness;

  bool operator==(const Ticket &lhs, const Ticket &rhs) {
    return memcmp(lhs.bytes.data(), rhs.bytes.data(), lhs.bytes.size()) == 0;
  }

}  // namespace fc::primitives::ticket
