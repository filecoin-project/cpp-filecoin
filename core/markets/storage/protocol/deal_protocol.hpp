/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE_PROTOCOL_DEAL_PROTOCOL_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE_PROTOCOL_DEAL_PROTOCOL_HPP

#include <libp2p/peer/peer_id.hpp>
#include <libp2p/peer/protocol.hpp>
#include "primitives/cid/cid.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/types.hpp"
#include "storage/filestore/path.hpp"
#include "vm/actor/builtin/market/actor.hpp"

namespace fc::markets::storage {

  using libp2p::peer::PeerId;
  using primitives::DealId;
  using primitives::TokenAmount;
  using primitives::piece::UnpaddedPieceSize;
  using storage::filestore::Path;
  using vm::actor::builtin::market::ClientDealProposal;
  using vm::actor::builtin::market::StorageParticipantBalance;

  const libp2p::peer::Protocol kDealProtocolId = "/fil/storage/mk/1.0.1";

  struct DataRef {
    std::string transfer_type;
    CID root;
    // TODO?
    // Optional, will be recomputed from the data if not given
    boost::optional<CID> piece_cid;
    UnpaddedPieceSize piece_size;
  }

  enum class StorageDealStatus {
    // TODO get 1st
    StorageDealUnknown,
    StorageDealProposalNotFound,
    StorageDealProposalRejected,
    StorageDealProposalAccepted,
    StorageDealStaged,
    StorageDealSealing,
    StorageDealActive,
    StorageDealFailing,
    StorageDealNotFound,

    // Internal
    StorageDealFundsEnsured,  // Deposited funds as neccesary to create a deal,
                              // ready to move forward
    StorageDealValidating,    // Verifying that deal parameters are good
    StorageDealTransferring,  // Moving data
    StorageDealWaitingForData,  // Manual transfer
    StorageDealVerifyData,  // Verify transferred data - generate CAR / piece
                            // data
    StorageDealPublishing,  // Publishing deal to chain
    StorageDealError,       // deal failed with an unexpected error
    StorageDealCompleted  // on provider side, indicates deal is active and info
                          // for retrieval is recorded

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
  }

}  // namespace fc::markets::storage

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE_PROTOCOL_DEAL_PROTOCOL_HPP
