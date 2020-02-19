/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_TICKET_TICKET_CODEC_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_TICKET_TICKET_CODEC_HPP

#include <vector>

#include "codec/cbor/cbor.hpp"
#include "common/outcome.hpp"
#include "common/outcome_throw.hpp"
#include "primitives/ticket/ticket.hpp"

namespace fc::primitives::ticket {
  enum class TicketCodecError: int {
    INVALID_TICKET_LENGTH = 1, // ticket decode error, invalid data length
  };
}  // namespace fc::primitives::ticket

OUTCOME_HPP_DECLARE_ERROR(fc::primitives::ticket, TicketCodecError)

namespace fc::primitives::ticket {
  /**
   * @brief cbor-encodes Ticket instance
   * @tparam Stream cbor-encoder stream type
   * @param s stream reference
   * @param ticket Ticket const reference to encode
   * @return stream reference
   */
  CBOR_ENCODE(Ticket, ticket) {
    return s << (s.list() << ticket.bytes);
  }

  /**
   * @brief cbor-decodes Ticket instance
   * @tparam Stream cbor-decoder stream type
   * @param s stream reference
   * @param ticket Ticket instance reference to decode into
   * @return stream reference
   */
  CBOR_DECODE(Ticket, ticket) {
    std::vector<uint8_t> data{};
    s.list() >> data;
    if (data.size() != ticket.bytes.size()) {
      outcome::raise(TicketCodecError::INVALID_TICKET_LENGTH);
    }
    std::copy(data.begin(), data.end(), ticket.bytes.begin());
    return s;
  }

}  // namespace fc::primitives::ticket

#endif  //CPP_FILECOIN_CORE_PRIMITIVES_TICKET_TICKET_CODEC_HPP
