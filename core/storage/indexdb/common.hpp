/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_STORAGE_INDEXDB_COMMON_HPP
#define CPP_FILECOIN_STORAGE_INDEXDB_COMMON_HPP

#include <set>

#include "primitives/tipset/tipset_key.hpp"

namespace fc::storage::indexdb {

  using TipsetKey = primitives::tipset::TipsetKey;
  using TipsetHash = primitives::tipset::TipsetHash;
  using BranchId = uint64_t;
  using Height = uint64_t;

  constexpr BranchId kNoBranch = 0;
  constexpr BranchId kGenesisBranch = 1;

  enum class Error {
    INDEXDB_CANNOT_CREATE = 1,
    TIPSET_NOT_FOUND,
    INDEXDB_INVALID_ARGUMENT,
    INDEXDB_EXECUTE_ERROR,
    INDEXDB_DECODE_ERROR,
    GRAPH_LOAD_ERROR,
    NO_CURRENT_CHAIN,
    BRANCH_NOT_FOUND,
    BRANCH_IS_NOT_A_HEAD,
    CYCLE_DETECTED,
    BRANCH_IS_NOT_A_ROOT,
    LINK_HEIGHT_MISMATCH,
    UNEXPECTED_TIPSET_PARENT,
  };

  struct TipsetInfo {
    TipsetHash hash;
    BranchId branch = kNoBranch;
    Height height = 0;
    TipsetHash parent_hash;
    BranchId parent_branch = kNoBranch;

    // 2 CIDs here
  };

  struct BranchInfo {
    BranchId id = kNoBranch;
    TipsetHash top;
    Height top_height = 0;
    TipsetHash bottom;
    Height bottom_height = 0;
    BranchId parent = kNoBranch;

    /// Ultimate root branch. For all branches fully synchronized, root must be
    /// equal to kGenesisBranch
    BranchId root = kNoBranch;

    /// Height of this branch above its root branch
    unsigned fork_height = 0;

    std::set<BranchId> forks;
  };

  using Branches = std::vector<std::reference_wrapper<const BranchInfo>>;

}  // namespace fc::storage::indexdb

OUTCOME_HPP_DECLARE_ERROR(fc::storage::indexdb, Error);

#endif  // CPP_FILECOIN_STORAGE_INDEXDB_COMMON_HPP
