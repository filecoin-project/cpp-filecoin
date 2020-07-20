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

  struct HeadMsg {
    Tipset tipset;
    BigInt weight;
  };

  using BranchId = uint64_t;
  using Height = uint64_t;

  constexpr BranchId kNoBranch = 0;
  constexpr BranchId kGenesisBranch = 1;

  struct SplitBranch {
    BranchId old_id = kNoBranch;
    BranchId new_id = kNoBranch;
    Height above_height = 0;
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

    /// Height of this branch above its root branch
    // unsigned fork_height = 0;

    std::set<BranchId> forks;
  };

  /// Heads configuration changed callback. If both values are present then
  /// it means that 'added' replaces 'removed'
  using HeadCallback = std::function<void(boost::optional<TipsetHash> removed,
                                          boost::optional<TipsetHash> added)>;

}  // namespace fc::sync

OUTCOME_HPP_DECLARE_ERROR(fc::sync, Error);

#endif  // CPP_FILECOIN_SYNC_COMMON_HPP
