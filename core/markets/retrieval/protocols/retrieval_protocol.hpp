/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <vector>

#include <libp2p/peer/protocol.hpp>
#include "codec/cbor/cbor_decode_stream.hpp"
#include "codec/cbor/cbor_encode_stream.hpp"
#include "codec/cbor/streams_annotation.hpp"
#include "markets/retrieval/types.hpp"
#include "primitives/address/address.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/types.hpp"
#include "storage/ipld/selector.hpp"
#include "vm/actor/builtin/types/payment_channel/voucher.hpp"

namespace fc::markets::retrieval {
  using codec::cbor::CborDecodeStream;
  using codec::cbor::CborEncodeStream;
  using fc::storage::ipld::Selector;
  using primitives::DealId;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using vm::actor::builtin::types::payment_channel::SignedVoucher;

  /**
   * @struct Deal proposal params
   */
  struct DealProposalParams {
    Selector selector;
    boost::optional<CID> piece;

    /* Proposed price */
    TokenAmount price_per_byte;

    /* Number of bytes before the next payment */
    uint64_t payment_interval{};

    /* Rate at which payment interval value increases */
    uint64_t payment_interval_increase{};

    TokenAmount unseal_price;
  };

  struct DealProposalParamsV0_0_1 : public DealProposalParams {};

  CBOR_TUPLE(DealProposalParamsV0_0_1,
             selector,
             piece,
             price_per_byte,
             payment_interval,
             payment_interval_increase,
             unseal_price)

  struct DealProposalParamsV1_0_0 : public DealProposalParams {};

  inline CBOR2_ENCODE(DealProposalParamsV1_0_0) {
    auto m{CborEncodeStream::orderedMap()};
    m["Selector"] << v.selector;
    m["PieceCID"] << v.piece;
    m["PricePerByte"] << v.price_per_byte;
    m["PaymentInterval"] << v.payment_interval;
    m["PaymentIntervalIncrease"] << v.payment_interval_increase;
    m["UnsealPrice"] << v.unseal_price;
    return s << m;
  }
  inline CBOR2_DECODE(DealProposalParamsV1_0_0) {
    auto m{s.map()};
    CborDecodeStream::named(m, "Selector") >> v.selector;
    CborDecodeStream::named(m, "PieceCID") >> v.piece;
    CborDecodeStream::named(m, "PricePerByte") >> v.price_per_byte;
    CborDecodeStream::named(m, "PaymentInterval") >> v.payment_interval;
    CborDecodeStream::named(m, "PaymentIntervalIncrease")
        >> v.payment_interval_increase;
    CborDecodeStream::named(m, "UnsealPrice") >> v.unseal_price;
    return s;
  }

  /**
   * Deal proposal
   */
  struct DealProposal {
    virtual ~DealProposal() = default;

    /* Identifier of the requested item */
    CID payload_cid;

    /* Identifier of the deal, can be the same for the different clients */
    DealId deal_id{};

    /* Deal params */
    DealProposalParams params;

    /** Returns protocol id */
    virtual const std::string &getType() const = 0;
  };

  struct DealProposalV0_0_1 : public DealProposal {
    DealProposalV0_0_1() = default;
    DealProposalV0_0_1(CID payload_cid,
                       DealId deal_id,
                       DealProposalParams params) {
      this->payload_cid = std::move(payload_cid);
      this->deal_id = deal_id;
      this->params = std::move(params);
    }
    explicit DealProposalV0_0_1(const DealProposal &deal_proposal) {
      payload_cid = deal_proposal.payload_cid;
      deal_id = deal_proposal.deal_id;
      params = deal_proposal.params;
    }

    inline static const std::string type{"RetrievalDealProposal"};

    const std::string &getType() const override {
      return type;
    }
  };

  inline CBOR2_ENCODE(DealProposalV0_0_1) {
    return s << (CborEncodeStream::list()
                 << v.payload_cid << v.deal_id
                 << DealProposalParamsV0_0_1{v.params});
  }
  inline CBOR2_DECODE(DealProposalV0_0_1) {
    auto cbor_list{s.list()};
    cbor_list >> v.payload_cid;
    cbor_list >> v.deal_id;
    v.params = cbor_list.get<DealProposalParamsV0_0_1>();
    return s;
  }

  struct DealProposalV1_0_0 : public DealProposal {
    DealProposalV1_0_0() = default;
    DealProposalV1_0_0(CID payload_cid,
                       DealId deal_id,
                       DealProposalParams params) {
      this->payload_cid = std::move(payload_cid);
      this->deal_id = deal_id;
      this->params = std::move(params);
    }
    explicit DealProposalV1_0_0(const DealProposal &deal_proposal) {
      payload_cid = deal_proposal.payload_cid;
      deal_id = deal_proposal.deal_id;
      params = deal_proposal.params;
    }

    inline static const std::string type{"RetrievalDealProposal/1"};

    const std::string &getType() const override {
      return type;
    }
  };

  inline CBOR2_ENCODE(DealProposalV1_0_0) {
    auto m{CborEncodeStream::orderedMap()};
    m["PayloadCID"] << v.payload_cid;
    m["ID"] << v.deal_id;
    m["Params"] << DealProposalParamsV1_0_0{v.params};
    return s << m;
  }
  inline CBOR2_DECODE(DealProposalV1_0_0) {
    auto m{s.map()};
    CborDecodeStream::named(m, "PayloadCID") >> v.payload_cid;
    CborDecodeStream::named(m, "ID") >> v.deal_id;
    v.params =
        CborDecodeStream::named(m, "Params").get<DealProposalParamsV1_0_0>();
    return s;
  }

  /**
   * Deal proposal response
   */
  struct DealResponse {
    /* Current deal status */
    DealStatus status;

    /* Deal ID */
    DealId deal_id{};

    /* Required tokens amount */
    TokenAmount payment_owed;

    /* Optional message */
    std::string message;
  };

  struct DealResponseV0_0_1 : DealResponse {
    inline static const std::string type{"RetrievalDealResponse"};
  };

  CBOR_TUPLE(DealResponseV0_0_1, status, deal_id, payment_owed, message)

  struct DealResponseV1_0_0 : DealResponse {
    inline static const std::string type{"RetrievalDealResponse/1"};
  };

  inline CBOR2_ENCODE(DealResponseV1_0_0) {
    auto m{CborEncodeStream::orderedMap()};
    m["Status"] << v.status;
    m["ID"] << v.deal_id;
    m["PaymentOwed"] << v.payment_owed;
    m["Message"] << v.message;
    return s << m;
  }
  inline CBOR2_DECODE(DealResponseV1_0_0) {
    auto m{s.map()};
    CborDecodeStream::named(m, "Status") >> v.status;
    CborDecodeStream::named(m, "ID") >> v.deal_id;
    CborDecodeStream::named(m, "PaymentOwed") >> v.payment_owed;
    CborDecodeStream::named(m, "Message") >> v.message;
    return s;
  }

  /**
   * Payment for an in progress retrieval deal
   */
  struct DealPayment {
    DealId deal_id{};
    Address payment_channel;
    SignedVoucher payment_voucher;
  };

  struct DealPaymentV0_0_1 : public DealPayment {
    inline static const std::string type{"RetrievalDealPayment"};
  };

  CBOR_TUPLE(DealPaymentV0_0_1, deal_id, payment_channel, payment_voucher)

  struct DealPaymentV1_0_0 : public DealPayment {
    inline static const std::string type{"RetrievalDealPayment/1"};
  };


  inline CBOR2_ENCODE(DealPaymentV1_0_0) {
    auto m{CborEncodeStream::orderedMap()};
    m["ID"] << v.deal_id;
    m["PaymentChannel"] << v.payment_channel;
    m["PaymentVoucher"] << v.payment_voucher;
    return s << m;
  }
  inline CBOR2_DECODE(DealPaymentV1_0_0) {
    auto m{s.map()};
    CborDecodeStream::named(m, "ID") >> v.deal_id;
    CborDecodeStream::named(m, "PaymentChannel") >> v.payment_channel;
    CborDecodeStream::named(m, "PaymentVoucher") >> v.payment_voucher;
    return s;
  }

  struct State {
    explicit State(const DealProposalParams &params)
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
