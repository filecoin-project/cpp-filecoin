/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_TICKET_EPOST_TICKET_CODEC_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_TICKET_EPOST_TICKET_CODEC_HPP

#include <vector>

#include "codec/cbor/cbor.hpp"
#include "common/outcome.hpp"
#include "common/outcome_throw.hpp"
#include "primitives/ticket/epost_ticket.hpp"

namespace fc::primitives::ticket {
  enum class EPoSTTicketCodecError : int {
    INVALID_PARTIAL_LENGTH = 1,  // invalid length of field partial
    INVALID_POST_RAND_LENGTH,    // invalid length of field post_rand
  };
}  // namespace fc::primitives::ticket

OUTCOME_HPP_DECLARE_ERROR(fc::primitives::ticket, EPoSTTicketCodecError)

namespace fc::primitives::ticket {
  CBOR_ENCODE_TUPLE(EPostTicket, partial, sector_id, challenge_index)

  /**
   * @brief cbor-decode EPostTicket instance
   * @tparam Stream cbor-decoder stream type
   * @param s stream reference
   * @param ticket Ticket reference to decode into
   * @return stream reference
   */
  CBOR_DECODE(EPostTicket, ticket) {
    std::vector<uint8_t> data{};
    s.list() >> data >> ticket.sector_id >> ticket.challenge_index;
    if (data.size() != ticket.partial.size()) {
      outcome::raise(EPoSTTicketCodecError::INVALID_PARTIAL_LENGTH);
    }
    std::copy(data.begin(), data.end(), ticket.partial.begin());
    return s;
  }

  CBOR_ENCODE_TUPLE(EPostProof, proofs, post_rand, candidates)

  /**
   * @brief cbor-decodes EPostProof instance
   * @tparam Stream cbor-decoder stream type
   * @param s stream reference
   * @param epp EPostProof refefence to decode into
   * @return stream reference
   */
  CBOR_DECODE(EPostProof, epp) {
    std::vector<uint8_t> rand;
    s.list() >> epp.proofs >> rand >> epp.candidates;
    if (rand.size() != epp.post_rand.size()) {
      outcome::raise(EPoSTTicketCodecError::INVALID_POST_RAND_LENGTH);
    }
    std::copy(rand.begin(), rand.end(), epp.post_rand.begin());
    return s;
  }
}  // namespace fc::primitives::ticket

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_TICKET_EPOST_TICKET_CODEC_HPP
