/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_INDEX_DB_HPP
#define CPP_FILECOIN_SYNC_INDEX_DB_HPP

#include "branches.hpp"
#include "lru_cache.hpp"
#include "storage/buffer_map.hpp"

namespace fc::sync {

  /// Indexed tipset info
  struct TipsetInfo {
    /// Tipset key (hash + CIDs)
    TipsetKey key;

    /// Branch ID in tipset graph
    BranchId branch = kNoBranch;

    /// Tipset height
    Height height = 0;

    /// Parent hash
    TipsetHash parent_hash;
  };

  using TipsetInfoPtr = std::shared_ptr<TipsetInfo>;
  using TipsetInfoCPtr = std::shared_ptr<const TipsetInfo>;

  /// Backend, persistent storage
  class IndexDbBackend;

  /// Index DB maintains positions of downloaded tipset in graph.
  /// Dealing with forks and holes
  class IndexDb {
   public:
    explicit IndexDb(std::shared_ptr<IndexDbBackend> backend);

    /// Initializes the backend and returns graph entries
    outcome::result<std::map<BranchId, std::shared_ptr<BranchInfo>>> init();

    /// Indexes genesis tipset
    outcome::result<void> storeGenesis(const Tipset &genesis_tipset);

    /// Indexes a new tipset, and renames branches if needed, within a single tx
    outcome::result<void> store(
        TipsetInfoPtr info, const boost::optional<RenameBranch> &branch_rename);

    /// Returns true if tipset is indexed
    bool contains(const TipsetHash &hash);

    /// Returns index info by tipset hash
    /// \param hash Tipset hash
    /// \param error_if_not_found If true then 'not found' will return error in
    /// outcome
    /// \return Tipset info or error
    outcome::result<TipsetInfoCPtr> get(const TipsetHash &hash,
                                        bool error_if_not_found);

    /// Returns tipset info by branch and height
    outcome::result<TipsetInfoCPtr> get(BranchId branch, Height height);

    using WalkCallback = std::function<void(TipsetInfoCPtr info)>;

    outcome::result<void> walkForward(BranchId branch,
                                      Height from_height,
                                      Height to_height,
                                      size_t limit,
                                      const WalkCallback &cb);

    outcome::result<void> walkBackward(const TipsetHash &from,
                                       Height to_height,
                                       const WalkCallback &cb);

   private:
    using Cache = LRUCache<TipsetHash, TipsetInfo>;

    std::shared_ptr<IndexDbBackend> backend_;
    Cache cache_;
  };

}  // namespace fc::sync

#endif  // CPP_FILECOIN_SYNC_INDEX_DB_HPP
