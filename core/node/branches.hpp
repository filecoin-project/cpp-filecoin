/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_BRANCHES_HPP
#define CPP_FILECOIN_SYNC_BRANCHES_HPP

#include <set>

#include "common.hpp"

namespace fc::sync {

  /// Info used for assigning new branch ID when merging of splitting branches)
  struct RenameBranch {
    /// Old branch ID
    BranchId old_id = kNoBranch;

    /// New branch ID
    BranchId new_id = kNoBranch;

    /// Operation applicable to tipsets above this height only (splitting)
    Height above_height = 0;

    /// Branches are splitting
    bool split = false;
  };

  /// Branch info, effectively branch index
  struct BranchInfo {
    /// Branch ID, branch which contains genesis has ID=1
    BranchId id = kNoBranch;

    /// Top tipset of this branch
    TipsetHash top;

    /// Height of top tipset
    Height top_height = 0;

    /// Bottom tipset of this branch
    TipsetHash bottom;

    /// Height of bottom tipset
    Height bottom_height = 0;

    /// Parent branch ID
    BranchId parent = kNoBranch;

    /// Hash of top tipset in parent branch (if any)
    TipsetHash parent_hash;

    /// True if this branch originates from genesis w/o holes
    bool synced_to_genesis = false;

    /// Children, if any
    /// N.B. forks.size() == 1 is inconsistent state, such 2 branches must be
    /// merged
    std::set<BranchId> forks;
  };

  using BranchCPtr = std::shared_ptr<const BranchInfo>;

  /// Acyclic graph of tipset branches.
  /// In unsynced state, not all branches may be connected, they connect
  /// as soon as tipsets are downloaded.
  /// Genesis and its branch always exists in live node with branch ID==1
  /// Ids > 1 represent either forks or not-yet-downloaded pieces of blockchain
  class Branches {
    /// Ptr for internal use
    using BranchPtr = std::shared_ptr<BranchInfo>;

   public:
    enum class Error {
      BRANCHES_LOAD_ERROR = 1,
      BRANCHES_NO_GENESIS_BRANCH,
      BRANCHES_PARENT_EXPECTED,
      BRANCHES_NO_CURRENT_CHAIN,
      BRANCHES_BRANCH_NOT_FOUND,
      BRANCHES_HEAD_NOT_FOUND,
      BRANCHES_HEAD_NOT_SYNCED,
      BRANCHES_CYCLE_DETECTED,
      BRANCHES_STORE_ERROR,
      BRANCHES_HEIGHT_MISMATCH,
      BRANCHES_NO_COMMON_ROOT,
      BRANCHES_NO_ROUTE,
    };

    /// True if no branches yet there
    bool empty() const;

    /// Heads are branches with no children which are synced to genesis
    using Heads = std::map<TipsetHash, BranchPtr>;

    /// Returns all head branches
    const Heads &getAllHeads() const;

    /// Returns Branch at given height as per "current" chain variant
    /// setCurrentHead() mus be called prior to chose the "current" chain
    /// \param h Height
    /// \param must_exist If true, then "not found" is error
    /// \return Non-zero branch ID on success
    outcome::result<BranchId> getBranchAtHeight(Height h,
                                                bool must_exist) const;

    /// Selects the current chain, from genesis to given head
    /// Necessary to search for branch by height
    /// \param head_branch Current head assigned
    /// \param height Max height of current chain (may be <= top height of
    /// branch for some purposes)
    /// \return Success if head_branch is synced to genesis
    outcome::result<void> setCurrentHead(BranchId head_branch, Height height);

    /// Returns the highest common ancestor of a and b
    outcome::result<BranchCPtr> getCommonRoot(BranchId a, BranchId b) const;

    /// Returns route between branches (all intermediary branches included)
    /// If from==to then trivial result { from } is returned
    outcome::result<std::vector<BranchId>> getRoute(BranchId from,
                                                    BranchId to) const;

    /// Result of findStorePosition, where to store a tipset
    struct StorePosition {
      /// Branch assigned to tipset
      BranchId assigned_branch = kNoBranch;

      /// If not zero - tipset must be attached to the bottom of this branch
      BranchId at_bottom_of_branch = kNoBranch;

      /// If not zero - tipset must be attached to the top of this branch
      BranchId on_top_of_branch = kNoBranch;

      /// If value is set then branches rename operation is required
      /// within the same transaction (that's why those struct is needed)
      boost::optional<RenameBranch> rename;
    };

    /// Finds the position in graph where to store tipset given.
    /// \param tipset Tipset being stored
    /// \param parent_hash Tipset's parent hash (just not to calculate it twice)
    /// \param parent_branch Branch of parent tipset
    /// (may be zero if no parent yet in the storage)
    /// \param parent_height Parent's height
    /// \return Result which is used by index db to make insert tx
    outcome::result<StorePosition> findStorePosition(
        const Tipset &tipset,
        const TipsetHash &parent_hash,
        BranchId parent_branch,
        Height parent_height) const;

    /// Splits branches according to 'pos' instructions
    /// \param new_top Hash of lower branch's top tipset
    /// \param new_bottom Hash of upper branch's bottom tipset
    /// \param new_bottom_height Height of new_bottom
    /// \param pos Additional info about how to split
    void splitBranch(const TipsetHash &new_top,
                     const TipsetHash &new_bottom,
                     Height new_bottom_height,
                     const RenameBranch &pos);

    /// Changes that happened in head branches set when insert operation occured
    /// May be empty if heads remain unchanged
    struct HeadChanges {
      /// Heads disappeared
      std::vector<TipsetHash> removed;

      /// Heads appeared
      std::vector<TipsetHash> added;
    };

    /// Stores tipset (and changes the graph) according to 'pos' instructions
    /// \param tipset Tipset to store
    /// \param parent_hash Parent hash (just not to calculate twice)
    /// \param pos Info how to store
    /// \return Changes that happened in head branches
    HeadChanges storeTipset(const TipsetCPtr &tipset,
                            const TipsetHash &parent_hash,
                            const StorePosition &pos);

    /// Branch info by ID
    outcome::result<BranchCPtr> getBranch(BranchId id) const;

    /// The lowest branch if walk into ID's parents
    outcome::result<BranchCPtr> getRootBranch(BranchId id) const;

    /// Clears all
    void clear();

    /// Initializes the graph
    /// \param all_branches Result of reading from index db
    /// \return On success, heads found. Errors if all_branches is inconsistent
    outcome::result<HeadChanges> init(
        std::map<BranchId, BranchPtr> all_branches);

    /// Stores genesis tipset
    outcome::result<void> storeGenesis(const TipsetCPtr &genesis_tipset);

   private:
    /// Makes a new branch
    void newBranch(const TipsetHash &hash,
                   Height height,
                   const TipsetHash &parent_hash,
                   const StorePosition &pos);

    /// Merges branch and its parent, changes in heads, if occured, are written
    /// to 'changes'
    void mergeBranches(const BranchPtr &branch,
                       BranchPtr &parent_branch,
                       HeadChanges &changes);

    /// Internal fn which updates heads (when synced to genesis)
    void updateHeads(BranchPtr &branch, bool synced, HeadChanges &changes);

    /// Internal fn, branch info by id
    BranchPtr getBranch(BranchId id);

    /// Returns new unasigned branch id
    BranchId newBranchId() const;

    /// The whole graph
    std::map<BranchId, BranchPtr> all_branches_;

    /// Heads
    std::map<TipsetHash, BranchPtr> heads_;

    /// Roots which are not have parents, but not genesis (i.e. holes)
    std::map<TipsetHash, BranchPtr> unloaded_roots_;

    /// Genesis branch info
    BranchPtr genesis_branch_;

    /// Current blockchain version (route, branch, and max height)
    std::map<Height, BranchPtr> current_chain_;
    BranchId current_top_branch_ = kNoBranch;
    Height current_height_ = 0;
  };
}  // namespace fc::sync

OUTCOME_HPP_DECLARE_ERROR(fc::sync, Branches::Error);

#endif  // CPP_FILECOIN_SYNC_BRANCHES_HPP
