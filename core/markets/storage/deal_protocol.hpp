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
  using vm::actor::builtin::types::market::ClientDealProposal;
  using vm::actor::builtin::types::market::DealProposal;
  using vm::actor::builtin::types::market::DealState;
  using vm::actor::builtin::types::market::StorageParticipantBalance;

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

  CBOR_TUPLE(DataRef, transfer_type, root, piece_cid, piece_size)

  /** Protocol V1.1.0 with named encoding and extended DataRef */
  struct DataRefV1_1_0 : public DataRef {};

  inline CBOR2_ENCODE(DataRefV1_1_0) {
    auto m{CborEncodeStream::map()};
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

  /** Base MinerDeal */
  struct MinerDeal {
    ClientDealProposal client_deal_proposal;
    CID proposal_cid;
    boost::optional<CID> add_funds_cid;
    boost::optional<CID> publish_cid;
    PeerInfo client;
    StorageDealStatus state;
    Path piece_path;
    Path metadata_path;
    bool is_fast_retrieval;
    std::string message;
    DataRef ref;
    DealId deal_id;
  };

  struct MinerDealV1_0_1 : public MinerDeal {
   private:
    friend CborEncodeStream &operator<<(CborEncodeStream &,
                                        const MinerDealV1_0_1 &);
    friend CborDecodeStream &operator>>(CborDecodeStream &, MinerDealV1_0_1 &);
  };

  inline CBOR2_ENCODE(MinerDealV1_0_1) {
    s << v.client_deal_proposal << v.proposal_cid << v.add_funds_cid
      << v.publish_cid << v.client << v.state << v.piece_path << v.metadata_path
      << v.is_fast_retrieval << v.message << v.message << DataRefV1_0_1{v.ref}
      << v.deal_id;
    return s;
  }

  inline CBOR2_DECODE(MinerDealV1_0_1) {
    s >> v.client_deal_proposal;
    s >> v.proposal_cid;
    s >> v.add_funds_cid;
    s >> v.publish_cid;
    s >> v.client;
    s >> v.state;
    s >> v.piece_path;
    s >> v.metadata_path;
    s >> v.is_fast_retrieval;
    s >> v.message;
    DataRefV1_0_1 ref;
    s >> ref;
    v.ref = ref;
    s >> v.deal_id;
    return s;
  }

  struct MinerDealV1_1_0 : public MinerDeal {
   private:
    friend CborEncodeStream &operator<<(CborEncodeStream &,
                                        const MinerDealV1_1_0 &);
    friend CborDecodeStream &operator>>(CborDecodeStream &, MinerDealV1_1_0 &);
  };

  inline CBOR2_ENCODE(MinerDealV1_1_0) {
    return s << v.client_deal_proposal << v.proposal_cid << v.add_funds_cid
             << v.publish_cid << v.client << v.state << v.piece_path
             << v.metadata_path << v.is_fast_retrieval << v.message << v.message
             << DataRefV1_1_0{v.ref} << v.deal_id;
  }
  inline CBOR2_DECODE(MinerDealV1_1_0) {
    s >> v.client_deal_proposal;
    s >> v.proposal_cid;
    s >> v.add_funds_cid;
    s >> v.publish_cid;
    s >> v.client;
    s >> v.state;
    s >> v.piece_path;
    s >> v.metadata_path;
    s >> v.is_fast_retrieval;
    s >> v.message;
    DataRefV1_1_0 ref;
    s >> ref;
    v.ref = ref;
    s >> v.deal_id;
    return s;
  }

  /** Base class */
  struct ClientDeal {
    ClientDealProposal client_deal_proposal;
    CID proposal_cid;
    boost::optional<CID> add_funds_cid;
    StorageDealStatus state;
    PeerInfo miner;
    Address miner_worker;
    DealId deal_id;
    DataRef data_ref;
    bool is_fast_retrieval;
    std::string message;
    CID publish_message;
  };

  CBOR_TUPLE(ClientDeal,
             client_deal_proposal,
             proposal_cid,
             add_funds_cid,
             state,
             miner,
             miner_worker,
             deal_id,
             data_ref,
             is_fast_retrieval,
             message,
             publish_message)

  /**
   * StorageDeal is a local combination of a proposal and a current deal state
   */
  struct StorageDeal {
    DealProposal proposal;
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

  struct ProposalV1_0_1 : public Proposal {
   private:
    friend CborEncodeStream &operator<<(CborEncodeStream &,
                                        const ProposalV1_0_1 &);
    friend CborDecodeStream &operator>>(CborDecodeStream &, ProposalV1_0_1 &);
  };

  inline CBOR2_ENCODE(ProposalV1_0_1) {
    return s << v.deal_proposal << DataRefV1_0_1{v.piece}
             << v.is_fast_retrieval;
  }
  inline CBOR2_DECODE(ProposalV1_0_1) {
    s >> v.deal_proposal;
    DataRefV1_0_1 piece;
    v.piece = piece;
    s >> piece;
    s >> v.is_fast_retrieval;
    return s;
  }

  struct ProposalV1_1_0 : public Proposal {
   private:
    friend CborEncodeStream &operator<<(CborEncodeStream &,
                                        const ProposalV1_1_0 &);
    friend CborDecodeStream &operator>>(CborDecodeStream &, ProposalV1_1_0 &);
  };

  inline CBOR2_ENCODE(ProposalV1_1_0) {
    auto m{CborEncodeStream::map()};
    m["DealProposal"] << v.deal_proposal;
    m["Piece"] << DataRefV1_1_0{v.piece};
    m["FastRetrieval"] << v.is_fast_retrieval;
    return s << m;
  }

  inline CBOR2_DECODE(ProposalV1_1_0) {
    auto m{s.map()};
    CborDecodeStream::named(m, "DealProposal") >> v.deal_proposal;
    DataRefV1_1_0 piece;
    CborDecodeStream::named(m, "Piece") >> piece;
    v.piece = piece;
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

  struct ResponseV1_0_1 : public Response {
   private:
    friend CborEncodeStream &operator<<(CborEncodeStream &,
                                        const ResponseV1_0_1 &);
    friend CborDecodeStream &operator>>(CborDecodeStream &, ResponseV1_0_1 &);
  };

  inline CBOR2_ENCODE(ResponseV1_0_1) {
    return s << v.state << v.message << v.proposal << v.publish_message;
  }
  inline CBOR2_DECODE(ResponseV1_0_1) {
    s >> v.state;
    s >> v.message;
    s >> v.proposal;
    s >> v.publish_message;
    return s;
  }

  struct ResponseV1_1_0 : public Response {
   private:
    friend CborEncodeStream &operator<<(CborEncodeStream &,
                                        const ResponseV1_1_0 &);
    friend CborDecodeStream &operator>>(CborDecodeStream &, ResponseV1_1_0 &);
  };

  inline CBOR2_ENCODE(ResponseV1_1_0) {
    auto m{CborEncodeStream::map()};
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
      return codec::cbor::encode(*this);
    };

   private:
    friend CborEncodeStream &operator<<(CborEncodeStream &,
                                        const SignedResponseV1_0_1 &);
    friend CborDecodeStream &operator>>(CborDecodeStream &, SignedResponseV1_0_1 &);
  };

  inline CBOR2_ENCODE(SignedResponseV1_0_1) {
    return s << ResponseV1_0_1{v.response} << v.signature;
  }
  inline CBOR2_DECODE(SignedResponseV1_0_1) {
    ResponseV1_0_1 response;
    s >> response;
    v.response = response;
    s >> v.signature;
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
      return codec::cbor::encode(*this);
    };

   private:
    friend CborEncodeStream &operator<<(CborEncodeStream &,
                                        const SignedResponseV1_1_0 &);
    friend CborDecodeStream &operator>>(CborDecodeStream &, SignedResponseV1_1_0 &);
  };

  inline CBOR2_ENCODE(SignedResponseV1_1_0) {
    auto m{CborEncodeStream::map()};
    m["Response"] << ResponseV1_1_0{v.response};
    m["Signature"] << v.signature;
    return s << m;
  }
  inline CBOR2_DECODE(SignedResponseV1_1_0) {
    auto m{s.map()};
    ResponseV1_1_0 response;
    CborDecodeStream::named(m, "Response") >> response;
    v.response = response;
    CborDecodeStream::named(m, "Signature") >> v.signature;
    return s;
  }

}  // namespace fc::markets::storage
