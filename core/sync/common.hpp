/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_COMMON_HPP
#define CPP_FILECOIN_SYNC_COMMON_HPP

#include <libp2p/peer/peer_id.hpp>

#include "primitives/tipset/tipset.hpp"
#include "primitives/big_int.hpp"

namespace fc::sync {

  enum class Error {
    SYNC_NOT_INITIALIZED = 1,
    SYNC_DATA_INTEGRITY_ERROR = 2,
    SYNC_UNEXPECTED_OBJECT_STATE = 3,
    SYNC_NO_PEERS = 4,
    SYNC_BAD_TIPSET = 5,
    SYNC_BAD_BLOCK = 6,
    SYNC_PUBSUB_FAILURE = 7,
    SYNC_MSG_LOAD_FAILURE = 8,
    SYNC_INCONSISTENT_BLOCKSYNC_RESPONSE = 9,
    SYNC_INCOMPLETE_BLOCKSYNC_RESPONSE = 10,
    SYNC_BLOCKSYNC_RESPONSE_ERROR = 11,
  };

  using crypto::signature::Signature;
  using primitives::block::Block;
  using primitives::block::BlockHeader;
  using primitives::block::MsgMeta;
  using primitives::block::BlockMsg;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetHash;
  using primitives::tipset::TipsetKey;
  using primitives::BigInt;
  using vm::message::UnsignedMessage;
  using vm::message::SignedMessage;
  using PeerId = libp2p::peer::PeerId;

  struct HeadMsg {
    Tipset tipset;
    BigInt weight;
  };

}  // namespace fc::sync

OUTCOME_HPP_DECLARE_ERROR(fc::sync, Error);

#endif  // CPP_FILECOIN_SYNC_COMMON_HPP
