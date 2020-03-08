/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TEST_TESTUTIL_PRIMITIVES_TICKET_PRINTER_HPP
#define CPP_FILECOIN_TEST_TESTUTIL_PRIMITIVES_TICKET_PRINTER_HPP

#include <iosfwd>

#include <boost/algorithm/string.hpp>
#include "filecoin/common/hexutil.hpp"
#include "filecoin/primitives/ticket/epost_ticket.hpp"
#include "filecoin/primitives/ticket/ticket.hpp"

/**
 * @brief human-readable print Ticket
 */
std::string print(const fc::primitives::ticket::Ticket &t);

/**
 * @brief human-readable print EPostTicket
 */

std::string print(const fc::primitives::ticket::EPostTicket &t);

/**
 * @brief human-readable prints EPostProof
 */
std::string print(const fc::primitives::ticket::EPostProof &p);

#endif  // CPP_FILECOIN_TEST_TESTUTIL_PRIMITIVES_TICKET_PRINTER_HPP
