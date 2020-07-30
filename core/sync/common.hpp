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
    SYNC_NO_GENESIS,
    SYNC_GENESIS_MISMATCH,
    SYNC_DATA_INTEGRITY_ERROR,
    SYNC_UNEXPECTED_OBJECT_STATE,
    SYNC_NO_PEERS,
    SYNC_BAD_TIPSET,
    SYNC_BAD_BLOCK,
    SYNC_PUBSUB_FAILURE,
    SYNC_MSG_LOAD_FAILURE,
    SYNC_INCONSISTENT_BLOCKSYNC_RESPONSE,
    SYNC_INCOMPLETE_BLOCKSYNC_RESPONSE,
    SYNC_BLOCKSYNC_RESPONSE_ERROR,

    BRANCHES_LOAD_ERROR,
    BRANCHES_NO_GENESIS_BRANCH,
    BRANCHES_PARENT_EXPECTED,
    BRANCHES_NO_CURRENT_CHAIN,
    BRANCHES_BRANCH_NOT_FOUND,
    BRANCHES_HEAD_NOT_FOUND,
    BRANCHES_HEAD_NOT_SYNCED,
    BRANCHES_CYCLE_DETECTED,
    BRANCHES_STORE_ERROR,
    BRANCHES_HEIGHT_MISMATCH,

    INDEXDB_CANNOT_CREATE,
    INDEXDB_ALREADY_EXISTS,
    INDEXDB_EXECUTE_ERROR,
    INDEXDB_TIPSET_NOT_FOUND,
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

  using TipsetCPtr = std::shared_ptr<const Tipset>;

  struct HeadMsg {
    TipsetCPtr tipset;
    BigInt weight;
  };

  using BranchId = uint64_t;
  using Height = uint64_t;

  constexpr BranchId kNoBranch = 0;
  constexpr BranchId kGenesisBranch = 1;

  struct RenameBranch {
    BranchId old_id = kNoBranch;
    BranchId new_id = kNoBranch;
    Height above_height = 0;
    bool split = false;
  };

  struct BranchInfo {
    BranchId id = kNoBranch;
    TipsetHash top;
    Height top_height = 0;
    TipsetHash bottom;
    Height bottom_height = 0;
    BranchId parent = kNoBranch;
    TipsetHash parent_hash;

    bool synced_to_genesis = false;

    std::set<BranchId> forks;
  };

  using BranchCPtr = std::shared_ptr<const BranchInfo>;

  /// Heads configuration changed callback. If both values are present then
  /// it means that 'added' replaces 'removed'
  using HeadCallback = std::function<void(boost::optional<TipsetHash> removed,
                                          boost::optional<TipsetHash> added)>;



}  // namespace fc::sync

OUTCOME_HPP_DECLARE_ERROR(fc::sync, Error);

#endif  // CPP_FILECOIN_SYNC_COMMON_HPP
