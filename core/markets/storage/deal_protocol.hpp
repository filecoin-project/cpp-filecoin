/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/peer_info.hpp>
#include <libp2p/peer/protocol.hpp>
#include "codec/cbor/streams_annotation.hpp"
#include "crypto/signature/signature.hpp"
#include "primitives/address/address.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/types.hpp"
#include "storage/filestore/path.hpp"
#include "vm/actor/builtin/types/market/deal.hpp"

namespace fc::markets::storage {

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
  const libp2p::peer::Protocol kDealProtocolId_v1_1_1 = "/fil/storage/mk/1.1.1";

  const std::string kTransferTypeGraphsync = "graphsync";
  const std::string kTransferTypeManual = "manual";

  struct DataRef {
    std::string transfer_type;
    CID root;
    // Optional, will be recomputed from the data if not given
    boost::optional<CID> piece_cid;
    UnpaddedPieceSize piece_size;
  };

  CBOR_TUPLE(DataRef, transfer_type, root, piece_cid, piece_size)

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

  CBOR_TUPLE(MinerDeal,
             client_deal_proposal,
             proposal_cid,
             add_funds_cid,
             publish_cid,
             client,
             state,
             piece_path,
             metadata_path,
             is_fast_retrieval,
             message,
             ref,
             deal_id)

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
  struct Proposal {
    ClientDealProposal deal_proposal;
    DataRef piece;
    bool is_fast_retrieval = false;
  };

  CBOR_TUPLE(Proposal, deal_proposal, piece, is_fast_retrieval)

  /**
   * Response is a response to a proposal sent over the network
   */
  struct Response {
    StorageDealStatus state;

    // DealProposalRejected
    std::string message;
    CID proposal;

    // StorageDealProposalAccepted
    boost::optional<CID> _unused;
  };

  CBOR_TUPLE(Response, state, message, proposal, _unused)

  /**
   * SignedResponse is a response that is signed
   */
  struct SignedResponse {
    Response response;
    Signature signature;
  };

  CBOR_TUPLE(SignedResponse, response, signature)

  const libp2p::peer::Protocol kDealStatusProtocolId_v1_0_1{
      "/fil/storage/status/1.0.1"};
  const libp2p::peer::Protocol kDealStatusProtocolId_v1_1_1{
      "/fil/storage/status/1.1.1"};

  struct ProviderDealState {
    StorageDealStatus status{};
    std::string message;
    DealProposal proposal;
    CID proposal_cid;
    boost::optional<CID> add_funds_cid;
    boost::optional<CID> publish_cid;
    DealId id{};
    bool fast_retrieval{};
  };
  CBOR_TUPLE(ProviderDealState,
             status,
             message,
             proposal,
             proposal_cid,
             add_funds_cid,
             publish_cid,
             id,
             fast_retrieval)

  struct DealStatusRequest {
    CID proposal;
    Signature signature;
  };
  CBOR_TUPLE(DealStatusRequest, proposal, signature)

  struct DealStatusResponse {
    ProviderDealState state;
    Signature signature;
  };
  CBOR_TUPLE(DealStatusResponse, state, signature)
}  // namespace fc::markets::storage
