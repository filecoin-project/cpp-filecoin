/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TEST_TESTUTIL_PRIMITIVES_TICKET_UTILS_HPP
#define CPP_FILECOIN_TEST_TESTUTIL_PRIMITIVES_TICKET_UTILS_HPP

#include "filecoin/primitives/ticket/epost_ticket.hpp"
#include "filecoin/primitives/ticket/ticket.hpp"

#include <libp2p/crypto/random_generator/boost_generator.hpp>

/**
 * @class TicketGenerator provides methods for creating random tickets and
 * related objects
 */
class TicketGenerator {
 public:
  using CSPRNG = libp2p::crypto::random::CSPRNG;
  using Ticket = fc::primitives::ticket::Ticket;
  using EPostTicket = fc::primitives::ticket::EPostTicket;
  using EPostProof = fc::primitives::ticket::EPostProof;

  TicketGenerator();

  /**
   * @brief generates random ticket
   * @return generated instance
   */
  Ticket makeTicket();

  /**
   * @brief generates random EPostTicket
   * @return generated instance
   */
  EPostTicket makeEPostTicket();

  /**
   * @brief generates random EPoostProof
   * @param proof_size number of bytes in proof
   * @param candidates_count number of candidates
   * @return generated instance
   */
  EPostProof makeEPostProof(size_t proof_size, size_t candidates_count);

 private:
  std::shared_ptr<CSPRNG> random_;  ///< random bytes generator
};

#endif  // CPP_FILECOIN_TEST_TESTUTIL_PRIMITIVES_TICKET_UTILS_HPP
