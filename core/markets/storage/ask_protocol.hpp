/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE__PROTOCOL_ASK_PROTOCOL_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE__PROTOCOL_ASK_PROTOCOL_HPP

#include <libp2p/peer/protocol.hpp>
#include "primitives/address/address.hpp"
#include "primitives/piece/piece.hpp"

namespace fc::markets::protocol {

  using primitives::ChainEpoch;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::piece::PaddedPieceSize;

  const libp2p::peer::Protocol kAskProtocolId = "/fil/storage/ask/1.0.1";

  struct StorageDeal {
    // Price per GiB / Epoch
    TokenAmount price;
    PaddedPieceSize min_piece_size;
  };

  struct StorageAsk {
    // Price per GiB / Epoch
    TokenAmount price;
    PaddedPieceSize min_piece_size;
    Address miner;
    ChainEpoch timestamp;
    ChainEpoch expiry;
    uint64_t seq_no;
  }

  struct SignedStorageAsk {
    StorageAsk ask;
    // TODO
    //    Signature signature;
  }

}  // namespace fc::markets::protocol

#endif CPP_FILECOIN_CORE_MARKETS_STORAGE__PROTOCOL_ASK_PROTOCOL_HPP
