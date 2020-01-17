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

  /**
   * @brief cbor-encode EPostTicket instance
   * @tparam Stream cbor-encoder stream type
   * @param s stream reference
   * @param ticket Ticket const reference to encode
   * @return stream reference
   */
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference<Stream>::type::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const EPostTicket &ticket) noexcept {
    return s << (s.list() << ticket.partial << ticket.sector_id
                          << ticket.challenge_index);
  }

  /**
   * @brief cbor-decode EPostTicket instance
   * @tparam Stream cbor-decoder stream type
   * @param s stream reference
   * @param ticket Ticket reference to decode into
   * @return stream reference
   */
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference<Stream>::type::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, EPostTicket &ticket) {
    std::vector<uint8_t> data{};
    s.list() >> data >> ticket.sector_id >> ticket.challenge_index;
    if (data.size() != ticket.partial.size()) {
      outcome::raise(EPoSTTicketCodecError::INVALID_PARTIAL_LENGTH);
    }
    std::copy(data.begin(), data.end(), ticket.partial.begin());
    return s;
  }

  /**
   * @brief cbor-encodes EPostProof instance
   * @tparam Stream cbor-encoder stream type
   * @param s stream reference
   * @param epp EPostProof const reference to encode
   * @return stream reference
   */
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference<Stream>::type::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const EPostProof &epp) noexcept {
    return s << (s.list() << epp.proof << epp.post_rand
                          << epp.candidates);
  }

  /**
   * @brief cbor-decodes EPostProof instance
   * @tparam Stream cbor-decoder stream type
   * @param s stream reference
   * @param epp EPostProof refefence to decode into
   * @return stream reference
   */
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference<Stream>::type::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, EPostProof &epp) {
    std::vector<uint8_t> proof, rand;
    s.list() >> proof >> rand >> epp.candidates;
    epp.proof = common::Buffer(std::move(proof));
    if (rand.size() != epp.post_rand.size()) {
      outcome::raise(EPoSTTicketCodecError::INVALID_POST_RAND_LENGTH);
    }
    std::copy(rand.begin(), rand.end(), epp.post_rand.begin());
    return s;
  }
}  // namespace fc::primitives::ticket

/**
 * @brief tickets encode/decode Outcome errors declaration
 */
OUTCOME_HPP_DECLARE_ERROR(fc::primitives::ticket, EPoSTTicketCodecError)

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_TICKET_EPOST_TICKET_CODEC_HPP
