/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_PROTOCOL_HPP
#define CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_PROTOCOL_HPP

#include <string>
#include <vector>

#include <libp2p/peer/protocol.hpp>
#include "codec/cbor/streams_annotation.hpp"
#include "markets/retrieval/types.hpp"
#include "primitives/address/address.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/types.hpp"

namespace fc::markets::retrieval {
  using primitives::TokenAmount;
  using primitives::address::Address;

  /*
   * @brief Retrieval Protocol ID V0
   */
  const libp2p::peer::Protocol kRetrievalProtocolId = "/fil/retrieval/0.0.1";

  /**
   * @struct Deal proposal params
   */
  struct DealProposalParams {
    /* Proposed price */
    TokenAmount price_per_byte;

    /* Number of bytes before the next payment */
    uint64_t payment_interval;

    /* Rate at which payment interval value increases */
    uint64_t payment_interval_increase;
  };

  CBOR_TUPLE(DealProposalParams,
             price_per_byte,
             payment_interval,
             payment_interval_increase);

  /**
   * Deal proposal
   */
  struct DealProposal {
    /* Identifier of the requested item */
    CID payload_cid;

    /* Identifier of the deal, can be the same for the different clients */
    uint64_t deal_id;

    /* Deal params */
    DealProposalParams params;
  };

  CBOR_TUPLE(DealProposal, payload_cid, deal_id, params);

  /**
   * Deal proposal response
   */
  struct DealResponse {
    /* Current deal status */
    DealStatus status;

    /* Deal ID */
    uint64_t deal_id;

    /* Required tokens amount */
    TokenAmount payment_owed;

    /* Optional message */
    std::string message;

    /* Requested data */
    std::vector<Block> blocks;
  };

  CBOR_TUPLE(DealResponse, status, deal_id, payment_owed, message, blocks);
}  // namespace fc::markets::retrieval

#endif
