/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/ticket/epost_ticket.hpp"

namespace fc::primitives::ticket {
  bool operator==(const EPostTicket &lhs, const EPostTicket &rhs) {
    return lhs.partial == rhs.partial && lhs.sector_id == rhs.sector_id
           && lhs.challenge_index == rhs.challenge_index;
  }

  bool operator==(const EPostProof &lhs, const EPostProof &rhs) {
    return lhs.proof == rhs.proof && lhs.post_rand == rhs.post_rand
           && lhs.candidates == rhs.candidates;
  }
}  // namespace fc::primitives::ticket
