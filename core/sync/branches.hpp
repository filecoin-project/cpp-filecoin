/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_BRANCHES_HPP
#define CPP_FILECOIN_SYNC_BRANCHES_HPP

#include "common.hpp"

namespace fc::sync {

  using TipsetCPtr = std::shared_ptr<const Tipset>;
  using BranchCPtr = std::shared_ptr<const BranchInfo>;

  class Branches {
    using BranchPtr = std::shared_ptr<BranchInfo>;

   public:
    bool empty() const;

    using Heads = std::map<TipsetHash, BranchPtr>;

    const Heads &getAllHeads() const;

    outcome::result<BranchId> getBranchAtHeight(Height h,
                                                bool must_exist) const;

    outcome::result<void> setCurrentHead(BranchId head_branch, Height height);

    struct StorePosition {
      BranchId assigned_branch = kNoBranch;
      BranchId at_bottom_of_branch = kNoBranch;
      BranchId on_top_of_branch = kNoBranch;
      boost::optional<SplitBranch> split;
    };

    outcome::result<StorePosition> findStorePosition(
        const Tipset &tipset,
        const TipsetHash &parent_hash,
        BranchId parent_branch,
        Height parent_height) const;

    struct HeadChange {
      boost::optional<TipsetHash> removed;
      boost::optional<TipsetHash> added;
    };

    void splitBranch(const TipsetHash &new_top,
                     const TipsetHash &new_bottom,
                     Height new_bottom_height,
                     const SplitBranch &pos);

    std::vector<HeadChange> storeTipset(TipsetCPtr tipset,
                                        const TipsetHash &parent_hash,
                                        const StorePosition &pos);

    void clear();

    outcome::result<std::vector<HeadChange>> init(
        std::map<BranchId, BranchPtr> all_branches);

   private:
    void newBranch(const TipsetHash &hash,
                   Height height,
                   const TipsetHash &parent_hash,
                   const StorePosition &pos);

    void mergeBranches(const BranchPtr &branch,
                       BranchPtr &parent_branch,
                       std::vector<HeadChange> &changes);

    void updateHeads(BranchPtr &branch,
                     bool synced,
                     std::vector<HeadChange> &changes);

    outcome::result<BranchCPtr> getBranch(BranchId id) const;

    BranchPtr getBranch(BranchId id);

    BranchId newBranchId() const;


    std::map<BranchId, BranchPtr> all_branches_;
    std::map<TipsetHash, BranchPtr> heads_;
    std::map<TipsetHash, BranchPtr> unloaded_roots_;
    BranchPtr genesis_branch_;
    std::map<Height, BranchPtr> current_chain_;
    BranchId current_top_branch_ = kNoBranch;
    Height current_height_ = 0;
  };
}  // namespace fc::sync

#endif  // CPP_FILECOIN_SYNC_BRANCHES_HPP
