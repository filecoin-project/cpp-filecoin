/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE__PROTOCOL_DEAL_PROTOCOL_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE__PROTOCOL_DEAL_PROTOCOL_HPP

#include <libp2p/peer/peer_id.hpp>
#include <libp2p/peer/protocol.hpp>
#include "crypto/signature/signature.hpp"
#include "primitives/address/address.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/types.hpp"
#include "storage/filestore/path.hpp"
#include "vm/actor/builtin/market/actor.hpp"

namespace fc::markets::storage {

  using crypto::signature::Signature;
  using libp2p::peer::PeerId;
  using primitives::DealId;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::piece::UnpaddedPieceSize;
  using storage::filestore::Path;
  using vm::actor::builtin::market::ClientDealProposal;
  using vm::actor::builtin::market::DealProposal;
  using vm::actor::builtin::market::DealState;
  using vm::actor::builtin::market::StorageParticipantBalance;

  const libp2p::peer::Protocol kDealProtocolId = "/fil/storage/mk/1.0.1";

  struct DataRef {
    std::string transfer_type;
    CID root;
    // TODO?
    // Optional, will be recomputed from the data if not given
    boost::optional<CID> piece_cid;
    UnpaddedPieceSize piece_size;
  };

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

    /** Publishing deal to chain */
    STORAGE_DEAL_PUBLISHING,

    /** deal failed with an unexpected error */
    STORAGE_DEAL_ERROR,

    /**
     * on provider side, indicates deal is active and info for retrieval is
     * recorded
     */
    STORAGE_DEAL_COMPLETED
  };

  struct MinerDeal : public ClientDealProposal {
    Cid proposal_cid;
    PeerId miner;
    PeerId client;
    StorageDealStatus state;
    Path piece_path;
    Path metadata_path;
    bool connection_closed;
    std::string message;
    DataRef ref;
    DealId deal_id;
  };

  struct ClientDeal : public ClientDealProposal {
    CID proposal_cid;
    StorageDealStatus state;
    PeerId miner;
    Address miner_worker;
    DealId deal_id;
    DataRef data_ref;
    std::string message;
    CID publish_message;
  };

  /**
   * StorageDeal is a local combination of a proposal and a current deal state
   */
  struct StorageDeal : public DealProposal, public DealState {};

  /**
   * Proposal is the data sent over the network from client to provider when
   * proposing a deal
   */
  struct Proposal {
    ClientDealProposal deal_proposal;
    DataRef piece;
  };

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

  /**
   * SignedResponse is a response that is signed
   */
  struct SignedResponse struct {
    Response response;
    Signature signature;
  };

}  // namespace fc::markets::storage

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE__PROTOCOL_DEAL_PROTOCOL_HPP
