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

  using fc::codec::cbor::CborEncodeStream;
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference<Stream>::type::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const Ticket &ticket) noexcept {
    return s << (s.list() << ticket.bytes);
  }

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference<Stream>::type::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, Ticket &ticket) {
    std::vector<uint8_t> data{};
    s >> data;
    if (data.size() != ticket.bytes.size()) {
      outcome::raise(TicketCodecError::INVALID_TICKET_LENGTH);
    }
    std::copy(data.begin(), data.end(), ticket.bytes.begin());
    return s;
  }

}  // namespace fc::primitives::ticket

/**
 * @brief tickets encode/decode Outcome errors declaration
 */
OUTCOME_HPP_DECLARE_ERROR(fc::primitives::ticket, TicketCodecError)


#endif  // CPP_FILECOIN_CORE_PRIMITIVES_TICKET_TICKET_CODEC_HPP
