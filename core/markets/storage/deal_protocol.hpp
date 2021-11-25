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

  const libp2p::peer::Protocol kDealProtocolId_v1_0_1 = "/fil/storage/mk/1.0.1";

  /** Protocol 1.1.0 uses named cbor */
  const libp2p::peer::Protocol kDealProtocolId_v1_1_1 = "/fil/storage/mk/1.1.0";

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
    /** Returns base DataRef */
    virtual const DataRef &ref() const = 0;
    DealId deal_id;
  };

  struct MinerDealV1_0_1 : public MinerDeal {
    const DataRef &ref() const override {
      return data_ref_;
    }

   private:
    friend CborEncodeStream &operator<<(CborEncodeStream &,
                                        const MinerDealV1_0_1 &);
    friend CborDecodeStream &operator>>(CborDecodeStream &, MinerDealV1_0_1 &);

    DataRefV1_0_1 data_ref_;
  };

  inline CBOR2_ENCODE(MinerDealV1_0_1) {
    return s << v.client_deal_proposal << v.proposal_cid << v.add_funds_cid
             << v.publish_cid << v.client << v.state << v.piece_path
             << v.metadata_path << v.is_fast_retrieval << v.message << v.message
             << v.data_ref_ << v.deal_id;
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
    s >> v.data_ref_;
    s >> v.deal_id;
    return s;
  }

  struct MinerDealV1_1_0 : public MinerDeal {
    const DataRef &ref() const override {
      return data_ref_;
    }

   private:
    friend CborEncodeStream &operator<<(CborEncodeStream &,
                                        const MinerDealV1_1_0 &);
    friend CborDecodeStream &operator>>(CborDecodeStream &, MinerDealV1_1_0 &);

    DataRefV1_1_0 data_ref_;
  };

  inline CBOR2_ENCODE(MinerDealV1_1_0) {
    return s << v.client_deal_proposal << v.proposal_cid << v.add_funds_cid
             << v.publish_cid << v.client << v.state << v.piece_path
             << v.metadata_path << v.is_fast_retrieval << v.message << v.message
             << v.data_ref_ << v.deal_id;
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
    s >> v.data_ref_;
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
   * proposing a deal
   */
  struct Proposal0 {
    struct Named;

    ClientDealProposal deal_proposal;
    DataRef piece;
    bool is_fast_retrieval = false;
  };

  CBOR_TUPLE(Proposal0, deal_proposal, piece, is_fast_retrieval)

  /**
   * Response is a response to a proposal sent over the network
   */
  struct Response {
    struct Named;

    StorageDealStatus state;

    // DealProposalRejected
    std::string message;
    CID proposal;

    // StorageDealProposalAccepted
    boost::optional<CID> _unused;
  };

  struct Response::Named : Response {};

  CBOR_TUPLE(Response, state, message, proposal, _unused)

  /**
   * SignedResponse is a response that is signed
   */
  struct SignedResponse {
    struct Named;

    Response response;
    Signature signature;
  };

  struct SignedResponse::Named : SignedResponse {};

  CBOR_TUPLE(SignedResponse, response, signature)
}  // namespace fc::markets::storage
