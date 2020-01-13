/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

//#include "primitives/ticket/ticket_codec.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::primitives::ticket, TicketCodecError, e) {
  using fc::primitives::ticket::TicketCodecError;
  switch (e) {
    case (TicketCodecError::INVALID_TICKET_LENGTH):
      return "Failed to decode ticket: invalid data length";
  }
}

namespace fc::primitives::ticket {}


