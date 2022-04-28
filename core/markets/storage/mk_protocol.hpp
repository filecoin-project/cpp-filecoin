/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/peer_info.hpp>
#include <libp2p/peer/protocol.hpp>
#include "codec/cbor/streams_annotation.hpp"
#include "common/libp2p/peer/cbor_peer_info.hpp"
#include "crypto/signature/signature.hpp"
#include "primitives/address/address.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/types.hpp"
#include "storage/filestore/path.hpp"
#include "vm/actor/builtin/types/market/deal.hpp"
#include "vm/actor/builtin/types/market/deal_proposal.hpp"
#include "vm/actor/builtin/types/universal/universal.hpp"

namespace fc::markets::storage {
  using codec::cbor::CborDecodeStream;
  using codec::cbor::CborEncodeStream;
  using crypto::signature::Signature;
  using ::fc::storage::filestore::Path;
  using libp2p::peer::PeerInfo;
  using primitives::DealId;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::piece::UnpaddedPieceSize;
  using vm::actor::builtin::types::Universal;
  using vm::actor::builtin::types::market::ClientDealProposal;
  using vm::actor::builtin::types::market::DealProposal;
  using vm::actor::builtin::types::market::DealState;

  const libp2p::peer::Protocol kDealMkProtocolId_v1_0_1 =
      "/fil/storage/mk/1.0.1";

  /** Protocol 1.1.0 uses named cbor */
  const libp2p::peer::Protocol kDealMkProtocolId_v1_1_0 =
      "/fil/storage/mk/1.1.0";

  const std::string kTransferTypeGraphsync = "graphsync";
  const std::string kTransferTypeManual = "manual";

  struct DataRef {
    std::string transfer_type;
    CID root;
    // Optional for non-manual transfer, will be recomputed from the data if not
    // given
    boost::optional<CID> piece_cid;
    // Optional for non-manual transfer, will be recomputed from the data if not
    // given
    UnpaddedPieceSize piece_size;
    // Optional: used as the denominator when calculating transfer
    uint64_t raw_block_size{};
  };

  /** For protocol v1.0.1. */
  struct DataRefV1_0_1 : public DataRef {};

  CBOR_TUPLE(DataRefV1_0_1, transfer_type, root, piece_cid, piece_size)

  /** Protocol V1.1.0 with named encoding and extended DataRef */
  struct DataRefV1_1_0 : public DataRef {};

  inline CBOR2_ENCODE(DataRefV1_1_0) {
    auto m{CborEncodeStream::orderedMap()};
    m["TransferType"] << v.transfer_type;
    m["Root"] << v.root;
    m["PieceCid"] << v.piece_cid;
    m["PieceSize"] << v.piece_size;
    m["RawBlockSize"] << v.raw_block_size;
    return s << m;
  }

  inline CBOR2_DECODE(DataRefV1_1_0) {
    auto m{s.map()};
    CborDecodeStream::named(m, "TransferType") >> v.transfer_type;
    CborDecodeStream::named(m, "Root") >> v.root;
    CborDecodeStream::named(m, "PieceCid") >> v.piece_cid;
    CborDecodeStream::named(m, "PieceSize") >> v.piece_size;
    CborDecodeStream::named(m, "RawBlockSize") >> v.raw_block_size;
    return s;
  }

  enum class StorageDealStatus : uint64_t {
    STORAGE_DEAL_UNKNOWN = 0,
    STORAGE_DEAL_PROPOSAL_NOT_FOUND,
    STORAGE_DEAL_PROPOSAL_REJECTED,
    STORAGE_DEAL_PROPOSAL_ACCEPTED,
    STORAGE_DEAL_STAGED,
    STORAGE_DEAL_SEALING,
    STORAGE_DEAL_FINALIZING,
    STORAGE_DEAL_ACTIVE,
    STORAGE_DEAL_EXPIRED,
    STORAGE_DEAL_SLASHED,
    STORAGE_DEAL_REJECTING,
    STORAGE_DEAL_FAILING,
    // Internal

    /** Deposited funds as necessary to create a deal, ready to move forward */
    STORAGE_DEAL_FUNDS_ENSURED,
    STORAGE_DEAL_CHECK_FOR_ACCEPTANCE,
    /** Verifying that deal parameters are good */
    STORAGE_DEAL_VALIDATING,
    STORAGE_DEAL_ACCEPT_WAIT,
    STORAGE_DEAL_START_DATA_TRANSFER,
    /** Moving data */
    STORAGE_DEAL_TRANSFERRING,

    /** Manual transfer */
    STORAGE_DEAL_WAITING_FOR_DATA,

    /** Verify transferred data - generate CAR / piece data */
    STORAGE_DEAL_VERIFY_DATA,

    /** Ensuring that provider collateral is sufficient */
    STORAGE_DEAL_ENSURE_PROVIDER_FUNDS,

    /** Ensuring that client funds are sufficient */
    STORAGE_DEAL_ENSURE_CLIENT_FUNDS,

    /** Waiting for funds to appear in Provider balance */
    STORAGE_DEAL_PROVIDER_FUNDING,

    /** Waiting for funds to appear in Client balance */
    STORAGE_DEAL_CLIENT_FUNDING,

    /** Publishing deal to chain */
    STORAGE_DEAL_PUBLISH,

    /** Waiting for deal to appear on chain */
    STORAGE_DEAL_PUBLISHING,

    /** deal failed with an unexpected error */
    STORAGE_DEAL_ERROR,
  };
  inline auto &classConversionMap(StorageDealStatus &&) {
    using E = StorageDealStatus;
    static fc::common::ConversionTable<E, 26> table{{
        {E::STORAGE_DEAL_UNKNOWN, "Unknown"},
        {E::STORAGE_DEAL_PROPOSAL_NOT_FOUND, "NotFound"},
        {E::STORAGE_DEAL_PROPOSAL_REJECTED, "ProposalRejected"},
        {E::STORAGE_DEAL_PROPOSAL_ACCEPTED, "ProposalAccepted"},
        {E::STORAGE_DEAL_STAGED, "Staged"},
        {E::STORAGE_DEAL_SEALING, "Sealing"},
        {E::STORAGE_DEAL_FINALIZING, "Finalizing"},
        {E::STORAGE_DEAL_ACTIVE, "Active"},
        {E::STORAGE_DEAL_EXPIRED, "Expired"},
        {E::STORAGE_DEAL_SLASHED, "Slashed"},
        {E::STORAGE_DEAL_REJECTING, "Rejecting"},
        {E::STORAGE_DEAL_FAILING, "Failing"},
        {E::STORAGE_DEAL_FUNDS_ENSURED, "FundsEnsured"},
        {E::STORAGE_DEAL_CHECK_FOR_ACCEPTANCE, "CheckForAcceptance"},
        {E::STORAGE_DEAL_VALIDATING, "DealValidating"},
        {E::STORAGE_DEAL_ACCEPT_WAIT, "AcceptWait"},
        {E::STORAGE_DEAL_START_DATA_TRANSFER, "StartDataTransfer"},
        {E::STORAGE_DEAL_TRANSFERRING, "DealTransferring"},
        {E::STORAGE_DEAL_WAITING_FOR_DATA, "WaitingForData"},
        {E::STORAGE_DEAL_VERIFY_DATA, "VerifyData"},
        {E::STORAGE_DEAL_ENSURE_PROVIDER_FUNDS, "EnsureProviderFunds"},
        {E::STORAGE_DEAL_PROVIDER_FUNDING, "ProviderFunding"},
        {E::STORAGE_DEAL_CLIENT_FUNDING, "ClientFunding"},
        {E::STORAGE_DEAL_PUBLISH, "DealPublish"},
        {E::STORAGE_DEAL_PUBLISHING, "DealPublishing"},
        {E::STORAGE_DEAL_ERROR, "Error"},
    }};
    return table;
  }

  /**
   * StorageDeal is a local combination of a proposal and a current deal state
   */
  struct StorageDeal {
    Universal<DealProposal> proposal;
    DealState state;
  };

  CBOR_TUPLE(StorageDeal, proposal, state)

  inline bool operator==(const StorageDeal &lhs, const StorageDeal &rhs) {
    return lhs.proposal == rhs.proposal and lhs.state == rhs.state;
  }

  /**
   * Proposal is the data sent over the network from client to provider when
   * proposing a deal.
   */
  struct Proposal {
    ClientDealProposal deal_proposal;
    DataRef piece;
    bool is_fast_retrieval = false;
  };

  struct ProposalV1_0_1 : public Proposal {};

  inline CBOR2_ENCODE(ProposalV1_0_1) {
    return s << (CborEncodeStream::list()
                 << v.deal_proposal << DataRefV1_0_1{v.piece}
                 << v.is_fast_retrieval);
  }
  inline CBOR2_DECODE(ProposalV1_0_1) {
    auto cbor_list{s.list()};
    cbor_list >> v.deal_proposal;
    v.piece = cbor_list.get<DataRefV1_0_1>();
    cbor_list >> v.is_fast_retrieval;
    return s;
  }

  struct ProposalV1_1_0 : public Proposal {};

  inline CBOR2_ENCODE(ProposalV1_1_0) {
    auto m{CborEncodeStream::orderedMap()};
    m["DealProposal"] << v.deal_proposal;
    m["Piece"] << DataRefV1_1_0{v.piece};
    m["FastRetrieval"] << v.is_fast_retrieval;
    return s << m;
  }

  inline CBOR2_DECODE(ProposalV1_1_0) {
    auto m{s.map()};
    CborDecodeStream::named(m, "DealProposal") >> v.deal_proposal;
    v.piece = CborDecodeStream::named(m, "Piece").get<DataRefV1_1_0>();
    CborDecodeStream::named(m, "FastRetrieval") >> v.is_fast_retrieval;
    return s;
  }

  /**
   * Response is a response to a proposal sent over the network
   */
  struct Response {
    StorageDealStatus state{};

    // DealProposalRejected
    std::string message;
    CID proposal;

    // StorageDealProposalAccepted - not used since V1.1.0
    boost::optional<CID> publish_message;
  };

  struct ResponseV1_0_1 : public Response {};

  CBOR_TUPLE(ResponseV1_0_1, state, message, proposal, publish_message)

  struct ResponseV1_1_0 : public Response {};

  inline CBOR2_ENCODE(ResponseV1_1_0) {
    auto m{CborEncodeStream::orderedMap()};
    m["State"] << v.state;
    m["Message"] << v.message;
    m["Proposal"] << v.proposal;
    m["PublishMessage"] << v.publish_message;
    return s << m;
  }

  inline CBOR2_DECODE(ResponseV1_1_0) {
    auto m{s.map()};
    CborDecodeStream::named(m, "State") >> v.state;
    CborDecodeStream::named(m, "Message") >> v.message;
    CborDecodeStream::named(m, "Proposal") >> v.proposal;
    CborDecodeStream::named(m, "PublishMessage") >> v.publish_message;
    return s;
  }

  /**
   * SignedResponse is a response that is signed
   */
  struct SignedResponse {
    virtual ~SignedResponse() = default;

    Response response;
    Signature signature;

    /** Returns response digset */
    virtual outcome::result<Bytes> getDigest() const = 0;
  };

  struct SignedResponseV1_0_1 : public SignedResponse {
    SignedResponseV1_0_1() = default;
    explicit SignedResponseV1_0_1(Response response) {
      this->response = std::move(response);
    }

    outcome::result<Bytes> getDigest() const override {
      return codec::cbor::encode(ResponseV1_0_1{this->response});
    };
  };

  inline CBOR2_ENCODE(SignedResponseV1_0_1) {
    return s << (CborEncodeStream::list()
                 << ResponseV1_0_1{v.response} << v.signature);
  }
  inline CBOR2_DECODE(SignedResponseV1_0_1) {
    auto cbor_list{s.list()};
    v.response = cbor_list.get<ResponseV1_0_1>();
    cbor_list >> v.signature;
    return s;
  }

  struct SignedResponseV1_1_0 : public SignedResponse {
    SignedResponseV1_1_0() = default;
    explicit SignedResponseV1_1_0(Response response) {
      this->response = std::move(response);
    }
    explicit SignedResponseV1_1_0(Response response, Signature signature) {
      this->response = std::move(response);
      this->signature = std::move(signature);
    }

    outcome::result<Bytes> getDigest() const override {
      return codec::cbor::encode(ResponseV1_1_0{this->response});
    };
  };

  inline CBOR2_ENCODE(SignedResponseV1_1_0) {
    auto m{CborEncodeStream::orderedMap()};
    m["Response"] << ResponseV1_1_0{v.response};
    m["Signature"] << v.signature;
    return s << m;
  }
  inline CBOR2_DECODE(SignedResponseV1_1_0) {
    auto m{s.map()};
    v.response = CborDecodeStream::named(m, "Response").get<ResponseV1_1_0>();
    CborDecodeStream::named(m, "Signature") >> v.signature;
    return s;
  }

}  // namespace fc::markets::storage
