/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/ticket/epost_ticket_codec.hpp"


OUTCOME_CPP_DEFINE_CATEGORY(fc::primitives::ticket, EPoSTTicketCodecError, e) {
  using fc::primitives::ticket::EPoSTTicketCodecError;
  switch (e) {
    case (EPoSTTicketCodecError::INVALID_PARTIAL_LENGTH):
      return "Invalid data size of field `partial`";
    case (EPoSTTicketCodecError::INVALID_POST_RAND_LENGTH):
      return "Invalid data size of field `post_rand`";
  }
}
