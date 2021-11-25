/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <utility>

#include "codec/cbor/streams_annotation.hpp"
#include "markets/storage/deal_protocol.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/types.hpp"
#include "vm/actor/builtin/types/market/deal.hpp"

namespace fc::markets::storage {
  using codec::cbor::CborDecodeStream;
  using codec::cbor::CborEncodeStream;
  using primitives::DealId;
  using vm::actor::builtin::types::market::DealProposal;

  const libp2p::peer::Protocol kDealStatusProtocolId_v1_0_1{
      "/fil/storage/status/1.0.1"};
  const libp2p::peer::Protocol kDealStatusProtocolId_v1_1_0{
      "/fil/storage/status/1.1.0"};

  /**
   * Base class for ProviderDealState
   */
  struct ProviderDealState {
    virtual ~ProviderDealState() = default;

    virtual outcome::result<Bytes> getDigest() const = 0;

    StorageDealStatus status{};
    std::string message;
    DealProposal proposal;
    CID proposal_cid;
    boost::optional<CID> add_funds_cid;
    boost::optional<CID> publish_cid;
    DealId id{};
    bool fast_retrieval{};
  };

  /** State used in V1.0.1 */
  struct ProviderDealStateV1_0_1 : public ProviderDealState {
    ProviderDealStateV1_0_1() = default;

    ProviderDealStateV1_0_1 &operator=(MinerDeal &&deal) {
      status = deal.state;
      message = std::move(deal.message);
      proposal = std::move(deal.client_deal_proposal.proposal);
      proposal_cid = std::move(deal.proposal_cid);
      add_funds_cid = std::move(deal.add_funds_cid);
      publish_cid = std::move(deal.publish_cid);
      id = deal.deal_id;
      fast_retrieval = deal.is_fast_retrieval;
      return *this;
    }

    outcome::result<Bytes> getDigest() const override {
      return codec::cbor::encode(*this);
    }
  };

  CBOR_TUPLE(ProviderDealStateV1_0_1,
             status,
             message,
             proposal,
             proposal_cid,
             add_funds_cid,
             publish_cid,
             id,
             fast_retrieval)

  /** State used in V1.1.0. Cbores with field names. */
  struct ProviderDealStateV1_1_0 : public ProviderDealState {
    ProviderDealStateV1_1_0() = default;

    ProviderDealStateV1_1_0(StorageDealStatus status,
                            std::string message,
                            DealProposal proposal,
                            CID proposal_cid,
                            boost::optional<CID> add_funds_cid,
                            boost::optional<CID> publish_cid,
                            DealId id,
                            bool fast_retrieval) {
      this->status = status;
      this->message = std::move(message);
      this->proposal = std::move(proposal);
      this->proposal_cid = std::move(proposal_cid);
      this->add_funds_cid = std::move(add_funds_cid);
      this->publish_cid = std::move(publish_cid);
      this->id = id;
      this->fast_retrieval = fast_retrieval;
    }

    ProviderDealStateV1_1_0 &operator=(MinerDeal &&deal) {
      status = deal.state;
      message = std::move(deal.message);
      proposal = std::move(deal.client_deal_proposal.proposal);
      proposal_cid = std::move(deal.proposal_cid);
      add_funds_cid = std::move(deal.add_funds_cid);
      publish_cid = std::move(deal.publish_cid);
      id = deal.deal_id;
      fast_retrieval = deal.is_fast_retrieval;
      return *this;
    }

    outcome::result<Bytes> getDigest() const override {
      return codec::cbor::encode(*this);
    }
  };

  inline CBOR2_ENCODE(ProviderDealStateV1_1_0) {
    auto m{CborEncodeStream::map()};
    m["State"] << v.status;
    m["Message"] << v.message;
    m["Proposal"] << v.proposal;
    m["ProposalCid"] << v.proposal_cid;
    m["AddFundsCid"] << v.add_funds_cid;
    m["PublishCid"] << v.publish_cid;
    m["DealID"] << v.id;
    m["FastRetrieval"] << v.fast_retrieval;
    return s << m;
  }

  inline CBOR2_DECODE(ProviderDealStateV1_1_0) {
    auto m{s.map()};
    CborDecodeStream::named(m, "State") >> v.status;
    CborDecodeStream::named(m, "Message") >> v.message;
    CborDecodeStream::named(m, "Proposal") >> v.proposal;
    CborDecodeStream::named(m, "ProposalCid") >> v.proposal_cid;
    CborDecodeStream::named(m, "AddFundsCid") >> v.add_funds_cid;
    CborDecodeStream::named(m, "PublishCid") >> v.publish_cid;
    CborDecodeStream::named(m, "DealID") >> v.id;
    CborDecodeStream::named(m, "FastRetrieval") >> v.fast_retrieval;
    return s;
  }

  /**
   * Base class for deal status request.
   */
  struct DealStatusRequest {
    /** Returns request digset, it is a cbor of proposal CID */
    outcome::result<Bytes> getDigest() const {
      return codec::cbor::encode(proposal);
    };

    CID proposal;
    Signature signature;
  };

  /** Request used in V1.0.1 */
  struct DealStatusRequestV1_0_1 : public DealStatusRequest {};

  CBOR_TUPLE(DealStatusRequestV1_0_1, proposal, signature)

  /** Request used in V1.1.0. Cbores with field names. */
  struct DealStatusRequestV1_1_0 : public DealStatusRequest {
    DealStatusRequestV1_1_0() = default;

    DealStatusRequestV1_1_0(CID proposal, Signature signature) {
      this->proposal = std::move(proposal);
      this->signature = std::move(signature);
    }
  };

  inline CBOR2_ENCODE(DealStatusRequestV1_1_0) {
    auto m{CborEncodeStream::map()};
    m["Proposal"] << v.proposal;
    m["Signature"] << v.signature;
    return s << m;
  }

  inline CBOR2_DECODE(DealStatusRequestV1_1_0) {
    auto m{s.map()};
    CborDecodeStream::named(m, "Proposal") >> v.proposal;
    CborDecodeStream::named(m, "Signature") >> v.signature;
    return s;
  }

  /**
   * Base class for deal response.
   * State depends on the protocol version.
   */
  struct DealStatusResponse {
    virtual ~DealStatusResponse() = default;
    virtual const ProviderDealState &state() const = 0;
    Signature signature;
  };

  /** Response used in V1.0.1 */
  struct DealStatusResponseV1_0_1 : public DealStatusResponse {
    DealStatusResponseV1_0_1() = default;
    DealStatusResponseV1_0_1(ProviderDealStateV1_0_1 state, Signature signature)
        : state_{std::move(state)} {
      this->signature = std::move(signature);
    }

    const ProviderDealState &state() const override {
      return state_;
    }

   private:
    friend CborEncodeStream &operator<<(CborEncodeStream &,
                                        const DealStatusResponseV1_0_1 &);
    friend CborDecodeStream &operator>>(CborDecodeStream &,
                                        DealStatusResponseV1_0_1 &);

    ProviderDealStateV1_0_1 state_;
  };

  inline CBOR2_ENCODE(DealStatusResponseV1_0_1) {
    return s << v.state_ << v.signature;
  }
  inline CBOR2_DECODE(DealStatusResponseV1_0_1) {
    s >> v.state_;
    s >> v.signature;
    return s;
  }

  /** Response used in V1.1.0 with named fields. */
  struct DealStatusResponseV1_1_0 : public DealStatusResponse {
    DealStatusResponseV1_1_0() = default;

    DealStatusResponseV1_1_0(ProviderDealStateV1_1_0 state, Signature signature)
        : state_{std::move(state)} {
      this->signature = std::move(signature);
    }

    const ProviderDealState &state() const override {
      return state_;
    }

   private:
    friend CborEncodeStream &operator<<(CborEncodeStream &,
                                        const DealStatusResponseV1_1_0 &);
    friend CborDecodeStream &operator>>(CborDecodeStream &,
                                        DealStatusResponseV1_1_0 &);
    ProviderDealStateV1_1_0 state_;
  };

  inline CBOR2_ENCODE(DealStatusResponseV1_1_0) {
    auto m{CborEncodeStream::map()};
    m["DealState"] << v.state_;
    m["Signature"] << v.signature;
    return s << m;
  }

  inline CBOR2_DECODE(DealStatusResponseV1_1_0) {
    auto m{s.map()};
    CborDecodeStream::named(m, "DealState") >> v.state_;
    CborDecodeStream::named(m, "Signature") >> v.signature;
    return s;
  }
}  // namespace fc::markets::storage
