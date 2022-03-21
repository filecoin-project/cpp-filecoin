/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/types/market/deal.hpp"
#include "markets/storage/mk_protocol.hpp"

namespace fc::markets::storage::client {
  using vm::actor::builtin::types::market::ClientDealProposal;
  using libp2p::peer::PeerInfo;
  using primitives::DealId;

  /** Internal state of a deal on the client side. */
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

}  // namespace fc::markets::storage::client
