/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_TICKET_TICKET_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_TICKET_TICKET_HPP

#include <libp2p/crypto/sha/sha256.hpp>
#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "common/outcome.hpp"
#include "crypto/randomness/randomness_types.hpp"
#include "crypto/vrf/vrf_types.hpp"

namespace fc::primitives::ticket {

  /**
   * @struct Ticket
   * https://github.com/filecoin-project/lotus/blob/2e95a536790113df7bcc95bc9215301fda23b65d/chain/types/blockheader.go#L19
   */
  struct Ticket {
    crypto::vrf::VRFProof bytes;
  };

  /**
   * @brief compares 2 tickets
   * @param lhs first ticket
   * @param rhs second ticket
   * @return true if equal false otherwise
   */
  bool operator==(const Ticket &lhs, const Ticket &rhs);

  /**
   * @brief compares two tickets alphabetically
   * @param lhs first ticket
   * @param rhs second ticket
   * @return true if lhs less thatn rhs, false othewise
   */
  bool operator<(const Ticket &lhs, const Ticket &rhs);

  /**
   * @brief draws randomness from ticket
   * @param ticket ticket
   * @param round round number
   * @return randomness value or error
   */
  outcome::result<crypto::randomness::Randomness> drawRandomness(
      const Ticket &ticket, int64_t round);

}  // namespace fc::primitives::ticket

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_TICKET_TICKET_HPP
