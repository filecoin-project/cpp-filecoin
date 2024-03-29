/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/peer_id.hpp>
#include "primitives/address/address.hpp"
#include "primitives/sector/sector.hpp"
#include "vm/actor/builtin/types/market/deal_proposal.hpp"

namespace fc::miner::types {
  using libp2p::peer::PeerId;
  using primitives::RegisteredSealProof;
  using primitives::SectorNumber;
  using primitives::SectorSize;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using vm::actor::builtin::types::Universal;
  using vm::actor::builtin::types::market::DealProposal;

  struct PreSealSector {
    CID comm_r;
    CID comm_d;
    SectorNumber sector_id;
    Universal<DealProposal> deal;
    RegisteredSealProof proof_type;
  };

  struct Miner {
    Address id;
    Address owner;
    Address worker;
    PeerId peer_id = common::kDefaultT<PeerId>();

    TokenAmount market_balance;
    TokenAmount power_balance;

    SectorSize sector_size{};

    std::vector<PreSealSector> sectors;
  };
}  // namespace fc::miner::types
