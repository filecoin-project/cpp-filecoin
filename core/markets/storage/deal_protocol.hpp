/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE__PROTOCOL_DEAL_PROTOCOL_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE__PROTOCOL_DEAL_PROTOCOL_HPP

#include <libp2p/peer/peer_info.hpp>
#include <libp2p/peer/protocol.hpp>
#include "codec/cbor/streams_annotation.hpp"
#include "crypto/signature/signature.hpp"
#include "primitives/address/address.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/types.hpp"
#include "storage/filestore/path.hpp"
#include "vm/actor/builtin/market/actor.hpp"

namespace fc::markets::storage {

  using crypto::signature::Signature;
  using ::fc::storage::filestore::Path;
  using libp2p::peer::PeerInfo;
  using primitives::DealId;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::piece::UnpaddedPieceSize;
  using vm::actor::builtin::market::ClientDealProposal;
  using vm::actor::builtin::market::DealProposal;
  using vm::actor::builtin::market::DealState;
  using vm::actor::builtin::market::StorageParticipantBalance;

  const libp2p::peer::Protocol kDealProtocolId = "/fil/storage/mk/1.0.1";

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

  enum class StorageDealStatus {
    STORAGE_DEAL_UNKNOWN = 0,
    STORAGE_DEAL_PROPOSAL_NOT_FOUND,
    STORAGE_DEAL_PROPOSAL_REJECTED,
    STORAGE_DEAL_PROPOSAL_ACCEPTED,
    STORAGE_DEAL_STAGED,
    STORAGE_DEAL_SEALING,
    STORAGE_DEAL_ACTIVE,
    STORAGE_DEAL_FAILING,
    STORAGE_DEAL_NOT_FOUND,

    // Internal

    /** Deposited funds as neccesary to create a deal, ready to move forward */
    STORAGE_DEAL_FUNDS_ENSURED,

    /** Verifying that deal parameters are good */
    STORAGE_DEAL_VALIDATING,

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

    /**
     * On provider side, indicates deal is active and info for retrieval is
     * recorded
     */
    STORAGE_DEAL_COMPLETED
  };

  struct MinerDeal {
    ClientDealProposal client_deal_proposal;
    CID proposal_cid;
    boost::optional<CID> add_funds_cid;
    boost::optional<CID> publish_cid;
    PeerInfo miner;
    PeerInfo client;
    StorageDealStatus state;
    Path piece_path;
    Path metadata_path;
    bool connection_closed;
    std::string message;
    DataRef ref;
    DealId deal_id;
  };

  CBOR_TUPLE(MinerDeal,
             client_deal_proposal,
             proposal_cid,
             add_funds_cid,
             publish_cid,
             miner,
             client,
             state,
             piece_path,
             metadata_path,
             connection_closed,
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

  /**
   * Proposal is the data sent over the network from client to provider when
   * proposing a deal
   */
  struct Proposal {
    ClientDealProposal deal_proposal;
    DataRef piece;
  };

  CBOR_TUPLE(Proposal, deal_proposal, piece)

  /**
   * Response is a response to a proposal sent over the network
   */
  struct Response {
    StorageDealStatus state;

    // DealProposalRejected
    std::string message;
    CID proposal;

    // StorageDealProposalAccepted
    CID publish_message;
  };

  CBOR_TUPLE(Response, state, message, proposal, publish_message)

  /**
   * SignedResponse is a response that is signed
   */
  struct SignedResponse {
    Response response;
    Signature signature;
  };

  CBOR_TUPLE(SignedResponse, response, signature)

}  // namespace fc::markets::storage

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE__PROTOCOL_DEAL_PROTOCOL_HPP
