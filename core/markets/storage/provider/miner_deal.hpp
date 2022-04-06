/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/libp2p/peer/cbor_peer_info.hpp"
#include "markets/storage/mk_protocol.hpp"
#include "vm/actor/builtin/types/market/deal.hpp"

namespace fc::markets::storage::provider {
  using common::kDefaultT;
  using vm::actor::builtin::types::market::ClientDealProposal;

  /** MinerDeal is an internal local state of a deal in a storage provider  */
  struct MinerDeal {
    ClientDealProposal client_deal_proposal;
    CID proposal_cid;
    boost::optional<CID> add_funds_cid;
    boost::optional<CID> publish_cid;
    PeerInfo client = kDefaultT<PeerInfo>();
    StorageDealStatus state;
    Path piece_path;
    Path metadata_path;
    bool is_fast_retrieval;
    std::string message;
    DataRef ref;
    DealId deal_id;
  };
}  // namespace fc::markets::storage::provider
