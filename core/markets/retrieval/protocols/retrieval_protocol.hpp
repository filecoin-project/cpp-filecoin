/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <vector>

#include <libp2p/peer/protocol.hpp>
#include "codec/cbor/streams_annotation.hpp"
#include "markets/retrieval/types.hpp"
#include "primitives/address/address.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/types.hpp"
#include "storage/ipld/selector.hpp"
#include "vm/actor/builtin/types/payment_channel/voucher.hpp"

namespace fc::markets::retrieval {
  using common::Buffer;
  using fc::storage::ipld::Selector;
  using primitives::DealId;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using vm::actor::builtin::types::payment_channel::SignedVoucher;

  /**
   * @struct Deal proposal params
   */
  struct DealProposalParams {
    struct Named;

    Selector selector;
    boost::optional<CID> piece;

    /* Proposed price */
    TokenAmount price_per_byte;

    /* Number of bytes before the next payment */
    uint64_t payment_interval;

    /* Rate at which payment interval value increases */
    uint64_t payment_interval_increase;

    TokenAmount unseal_price;
  };

  struct DealProposalParams::Named : DealProposalParams {};
  CBOR2_DECODE_ENCODE(DealProposalParams::Named)

  /**
   * Deal proposal
   */
  struct DealProposal {
    struct Named;

    /* Identifier of the requested item */
    CID payload_cid;

    /* Identifier of the deal, can be the same for the different clients */
    DealId deal_id;

    /* Deal params */
    DealProposalParams params;
  };

  struct DealProposal::Named : DealProposal {
    inline static const std::string type{"RetrievalDealProposal/1"};
  };
  CBOR2_DECODE_ENCODE(DealProposal::Named)

  /**
   * Deal proposal response
   */
  struct DealResponse {
    struct Named;

    /* Current deal status */
    DealStatus status;

    /* Deal ID */
    DealId deal_id;

    /* Required tokens amount */
    TokenAmount payment_owed;

    /* Optional message */
    std::string message;
  };

  struct DealResponse::Named : DealResponse {
    inline static const std::string type{"RetrievalDealResponse/1"};
  };
  CBOR2_DECODE_ENCODE(DealResponse::Named)

  /**
   * Payment for an in progress retrieval deal
   */
  struct DealPayment {
    struct Named;

    DealId deal_id = 0;
    Address payment_channel;
    SignedVoucher payment_voucher;
  };

  struct DealPayment::Named : DealPayment {
    inline static const std::string type{"RetrievalDealPayment/1"};
  };
  CBOR2_DECODE_ENCODE(DealPayment::Named)

  struct State {
    State(const DealProposalParams &params)
        : params{params},
          interval{params.payment_interval},
          owed{params.unseal_price} {}

    void block(uint64_t size) {
      assert(!owed);
      bytes += size;
      TokenAmount unpaid{bytes * params.price_per_byte
                         - (paid - params.unseal_price)};
      if (unpaid >= interval * params.price_per_byte) {
        owed = unpaid;
      }
    }

    void last() {
      owed = params.unseal_price + bytes * params.price_per_byte - paid;
    }

    void pay(const TokenAmount &amount) {
      assert(amount <= owed);
      paid += amount;
      owed -= amount;
      if (!owed) {
        interval += params.payment_interval_increase;
      }
    }

    DealProposalParams params;
    uint64_t interval, bytes{};
    TokenAmount paid, owed;
  };
}  // namespace fc::markets::retrieval
