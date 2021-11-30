/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "markets/storage/mk_protocol.hpp"
#include "vm/actor/builtin/types/market/deal.hpp"

namespace fc::markets::storage::provider {
  using vm::actor::builtin::types::market::ClientDealProposal;

  /** MinerDeal is an internal local state of a deal in a storage provider  */
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
}  // namespace fc::markets::storage::provider
