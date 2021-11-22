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

  /** Protocol 1.1.1 uses named cbor */
  const libp2p::peer::Protocol kAskProtocolId_v1_1_1 = "/fil/storage/ask/1.1.1";

  struct StorageAsk {
    struct Named;

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

  struct StorageAsk::Named : StorageAsk {};

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
    struct Named;

    StorageAsk ask;
    Signature signature;
  };

  struct SignedStorageAsk::Named : SignedStorageAsk {};

  CBOR_TUPLE(SignedStorageAsk, ask, signature)

  /**
   * AskRequest is a request for current ask parameters for a given miner
   */
  struct AskRequest {
    struct Named;

    Address miner;
  };

  /** Named ask request for named cbor */
  struct AskRequest::Named : AskRequest {};

  CBOR_TUPLE(AskRequest, miner)

  /**
   * AskResponse is the response sent over the network in response to an ask
   * request
   */
  struct AskResponse {
    struct Named;

    SignedStorageAsk ask;
  };

  /** Named ask response for named cbor */
  struct AskResponse::Named : AskResponse {};

  CBOR_TUPLE(AskResponse, ask)

}  // namespace fc::markets::storage
