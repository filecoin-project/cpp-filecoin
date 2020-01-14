/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_TICKET_EPOST_TICKET_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_TICKET_EPOST_TICKET_HPP

#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "crypto/vrf/vrf_types.hpp"

namespace fc::primitives::ticket {
  /**
   * @struct EPostTicket proof of space/time ticket
   * https://github.com/filecoin-project/lotus/blob/2e95a536790113df7bcc95bc9215301fda23b65d/chain/types/blockheader.go#L23
   */
  struct EPostTicket {
    common::Blob<32> partial;
    uint64_t sector_id{};
    uint64_t challenge_index{};
  };

  using PostRandomness = crypto::vrf::VRFResult;

  /**
   * @struct EPostProof proof of space/time
   */
  struct EPostProof {
    common::Buffer proof;      ///< number of bytes is not known beforehand
    PostRandomness post_rand;  ///< randomness
    std::vector<EPostTicket> candidates;  ///< candidates
  };

  /**
   * @brief compares 2 tickets
   * @param lhs first ticket
   * @param rhs second ticket
   * @return true if equal false otherwise
   */
  bool operator==(const EPostTicket &lhs, const EPostTicket &rhs);

  /**
   * @brief compares 2 proofs
   * @param lhs first proof
   * @param rhs second proof
   * @return true if equal false otherwise
   */
  bool operator==(const EPostProof &lhs, const EPostProof &rhs);

}  // namespace fc::primitives::ticket

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_TICKET_EPOST_TICKET_HPP
