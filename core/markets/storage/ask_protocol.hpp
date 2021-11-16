/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/protocol.hpp>
#include "codec/cbor/streams_annotation.hpp"
#include "crypto/signature/signature.hpp"
#include "primitives/address/address.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/types.hpp"

namespace fc::markets::storage {

  using crypto::signature::Signature;
  using primitives::ChainEpoch;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::piece::PaddedPieceSize;

  const libp2p::peer::Protocol kAskProtocolId_v1_0_1 = "/fil/storage/ask/1.0.1";
  const libp2p::peer::Protocol kAskProtocolId_v1_1_1 = "/fil/storage/ask/1.1.1";

  struct StorageAsk {
    // Price per GiB / Epoch
    TokenAmount price;
    TokenAmount verified_price;
    PaddedPieceSize min_piece_size;
    PaddedPieceSize max_piece_size;
    Address miner;
    ChainEpoch timestamp;
    ChainEpoch expiry;
    uint64_t seq_no;
  };

  CBOR_TUPLE(StorageAsk,
             price,
             verified_price,
             min_piece_size,
             max_piece_size,
             miner,
             timestamp,
             expiry,
             seq_no)

  struct SignedStorageAsk {
    StorageAsk ask;
    Signature signature;
  };

  CBOR_TUPLE(SignedStorageAsk, ask, signature)

  /**
   * AskRequest is a request for current ask parameters for a given miner
   */
  struct AskRequest {
    Address miner;
  };

  CBOR_TUPLE(AskRequest, miner)

  /**
   * AskResponse is the response sent over the network in response to an ask
   * request
   */
  struct AskResponse {
    SignedStorageAsk ask;
  };

  CBOR_TUPLE(AskResponse, ask)

}  // namespace fc::markets::storage
