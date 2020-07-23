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
#include "storage/ipld/selector.hpp"
#include "vm/actor/builtin/payment_channel/payment_channel_actor_state.hpp"

namespace fc::markets::retrieval {
  using common::Buffer;
  using fc::storage::ipld::Selector;
  using primitives::DealId;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using vm::actor::builtin::payment_channel::SignedVoucher;

  /*
   * @brief Retrieval Protocol ID V0
   */
  const libp2p::peer::Protocol kRetrievalProtocolId = "/fil/retrieval/0.0.1";

  /**
   * @struct Deal proposal params
   */
  struct DealProposalParams {
    Selector selector;
    boost::optional<CID> piece;

    /* Proposed price */
    TokenAmount price_per_byte;

    /* Number of bytes before the next payment */
    uint64_t payment_interval;

    /* Rate at which payment interval value increases */
    uint64_t payment_interval_increase;
  };

  CBOR_TUPLE(DealProposalParams,
             selector,
             piece,
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
    DealId deal_id;

    /* Deal params */
    DealProposalParams params;
  };

  CBOR_TUPLE(DealProposal, payload_cid, deal_id, params);

  /**
   * Deal proposal response
   */
  struct DealResponse {
    /// ipld block
    struct Block {
      /// CID bytes with multihash without hash bytes
      Buffer prefix;
      Buffer data;
    };

    /* Current deal status */
    DealStatus status;

    /* Deal ID */
    DealId deal_id;

    /* Required tokens amount */
    TokenAmount payment_owed;

    /* Optional message */
    std::string message;

    /* Requested data */
    std::vector<Block> blocks;
  };
  CBOR_TUPLE(DealResponse, status, deal_id, payment_owed, message, blocks)
  CBOR_TUPLE(DealResponse::Block, prefix, data)

  /**
   * Payment for an in progress retrieval deal
   */
  struct DealPayment {
    DealId deal_id;
    Address payment_channel;
    SignedVoucher payment_voucher;
  };
  CBOR_TUPLE(DealPayment, deal_id, payment_channel, payment_voucher);

}  // namespace fc::markets::retrieval

#endif
