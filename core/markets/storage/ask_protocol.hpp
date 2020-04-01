/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE__PROTOCOL_ASK_PROTOCOL_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE__PROTOCOL_ASK_PROTOCOL_HPP

#include <libp2p/peer/protocol.hpp>
#include "crypto/signature/signature.hpp"
#include "primitives/address/address.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/types.hpp"

namespace fc::markets::storage {

  using crypto::signature::Signature;
  using primitives::ChainEpoch;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::piece::PaddedPieceSize;

  const libp2p::peer::Protocol kAskProtocolId = "/fil/storage/ask/1.0.1";

  struct StorageAsk {
    // Price per GiB / Epoch
    TokenAmount price;
    PaddedPieceSize min_piece_size;
    Address miner;
    ChainEpoch timestamp;
    ChainEpoch expiry;
    uint64_t seq_no;
  };

  struct SignedStorageAsk {
    StorageAsk ask;
    Signature signature;
  };

  /**
   * AskRequest is a request for current ask parameters for a given miner
   */
  struct AskRequest {
    Address miner;
  };

  /**
   * AskResponse is the response sent over the network in response to an ask
   * request
   */
  struct AskResponse {
    SignedStorageAsk asl;
  };

}  // namespace fc::markets::storage

#endif CPP_FILECOIN_CORE_MARKETS_STORAGE__PROTOCOL_ASK_PROTOCOL_HPP
