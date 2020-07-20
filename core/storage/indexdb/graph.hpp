/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_STORAGE_INDEXDB_GRAPH_HPP
#define CPP_FILECOIN_STORAGE_INDEXDB_GRAPH_HPP

#include <map>

#include "common.hpp"

namespace fc::storage::indexdb {

  /// Graph of chain branches: auxiliary structure used by IndexDB
  class Graph {
   public:
    bool empty() const;

    Branches getRoots() const;

    Branches getHeads() const;

    BranchId getLastBranchId() const;

    outcome::result<std::reference_wrapper<const BranchInfo>> getBranch(
        BranchId branchId) const;

    /// Finds branch by height in current chain
    outcome::result<BranchId> findByHeight(Height height) const;

    outcome::result<void> load(std::map<BranchId, BranchInfo> all_branches);

    outcome::result<void> switchToHead(BranchId head);

    outcome::result<void> newBranch(BranchId branch_id,
                                    const TipsetHash &new_top,
                                    Height new_height);

    outcome::result<void> updateTop(BranchId branch_id,
                                    const TipsetHash &new_top,
                                    Height new_height);

    outcome::result<void> updateBottom(BranchId branch_id,
                                       const TipsetHash &new_top,
                                       Height new_height);

    /// merges successor into parent
    outcome::result<void> mergeBranches(BranchId parent_id,
                                        BranchId successor_id);

    outcome::result<void> linkBranches(BranchId parent_id,
                                        BranchId successor_id);

    outcome::result<void> splitBranch(BranchId branch_id,
                                      BranchId tail_branch_id,
                                      TipsetHash tail_bottom_tipset,
                                      Height tail_bottom_height);

    /// All removes can be performed from head only
    /// returns parent branch id and successor branch id
    outcome::result<std::pair<BranchId, BranchId>> removeHead(BranchId head);

    /// Returns branch id of the new base
//    outcome::result<BranchId> linkBranches(BranchId base_branch,
//                                           BranchId successor_branch,
//                                           TipsetHash base_tipset,
//                                           Height base_height);
//
//    outcome::result<void> linkToHead(BranchId base_branch,
//                                     BranchId successor_branch);
//
//    outcome::result<void> appendToHead(BranchId branch,
//                                       TipsetHash new_top,
//                                       Height new_top_height);

    void clear();

   private:
    BranchInfo &get(BranchId id);

    outcome::result<void> assignRootFields(unsigned max_fork_height,
                                           BranchId root_id,
                                           unsigned fork_height,
                                           BranchId id);

    Branches getRootsOrHeads(const std::set<BranchId> &ids) const;

    outcome::result<void> loadFailed();

    outcome::result<std::pair<BranchId, BranchId>> merge(BranchInfo &b);

    std::map<BranchId, BranchInfo> all_branches_;
    std::set<BranchId> roots_;
    std::set<BranchId> heads_;
    std::map<Height, BranchId> current_chain_;
    Height current_chain_bottom_height_ = 0;
  };

}  // namespace fc::storage::indexdb

#endif  // CPP_FILECOIN_STORAGE_INDEXDB_GRAPH_HPP
