/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "branches.hpp"
#include "index_db.hpp"
#include "storage/ipfs/datastore.hpp"

#ifndef CPP_FILECOIN_SYNC_CHAIN_DB_HPP
#define CPP_FILECOIN_SYNC_CHAIN_DB_HPP

namespace fc::sync {

  using TipsetCache = LRUCache<TipsetHash, const Tipset>;

  using IpfsStoragePtr = std::shared_ptr<storage::ipfs::IpfsDatastore>;

  /// Chain DB maintains tipset storage, cache, index, and graph (forks and
  /// holes). Used by ChainStore
  class ChainDb {
   public:
    enum class Error {
      CHAIN_DB_NOT_INITIALIZED = 1,
      CHAIN_DB_BAD_TIPSET,
      CHAIN_DB_NO_GENESIS,
      CHAIN_DB_GENESIS_MISMATCH,
      CHAIN_DB_DATA_INTEGRITY_ERROR,
    };

    /// Heads configuration changed callback. If both values are present then
    /// it means that 'added' replaces 'removed'
    using HeadCallback = std::function<void(std::vector<TipsetHash> removed,
                                            std::vector<TipsetHash> added)>;

    ChainDb();

    /// Initializes contained objects to consistent state, returns error if
    /// cannot init
    outcome::result<void> init(TsLoadPtr ts_load,
                               std::shared_ptr<IndexDb> index_db,
                               const boost::optional<CID> &genesis_cid,
                               bool creating_new_db);

    /// Assigns head change callback
    outcome::result<void> start(HeadCallback on_heads_changed);

    /// Returns success if state is consistent, or error otherwise
    outcome::result<void> stateIsConsistent() const;

    const CID &genesisCID() const;

    const Tipset &genesisTipset() const;

    /// Tipset's sync state
    struct SyncState {
      /// True if tipset is indexed
      bool tipset_indexed = false;

      /// True if the whole chain down from this tipset is indexed
      bool chain_indexed = false;

      /// If !chain_indexed, when this is the bottom of this subchain
      TipsetCPtr unsynced_bottom;
    };

    /// Returns sync state of given tipset
    outcome::result<SyncState> getSyncState(const TipsetHash &hash);

    /// Gets all heads via callback
    outcome::result<void> getHeads(const HeadCallback &callback);

    /// Returns true if the hash given is head
    bool isHead(const TipsetHash &hash);

    outcome::result<TipsetCPtr> getTipsetByHash(const TipsetHash &hash);

    outcome::result<TipsetCPtr> getTipsetByHeight(Height height);

    outcome::result<TipsetCPtr> getTipsetByKey(const TipsetKey &key);

    /// Returns highest common ancestor of a and b, if any.
    /// Used to make route of revert and apply state operations
    outcome::result<TipsetCPtr> findHighestCommonAncestor(const TipsetCPtr &a,
                                                          const TipsetCPtr &b);

    /// Assigns given head as 'current' for tipset-by-height operations
    outcome::result<void> setCurrentHead(const TipsetHash &head);

    /// returns true to proceed walking, false to stop
    using WalkCallback = std::function<bool(TipsetCPtr tipset)>;

    /// Walks forward through tipsets, limit is used because these steps are
    /// performed in event handlers, not to consume much CPU time at once,
    /// schedule next step instead
    outcome::result<void> walkForward(const TipsetCPtr &from,
                                      const TipsetCPtr &to,
                                      size_t limit,
                                      const WalkCallback &cb);

    /// Walks backwards throug tipset and its parents
    outcome::result<void> walkBackward(const TipsetHash &from,
                                       Height to_height,
                                       const WalkCallback &cb);

    /// Stores tipset in index db and graph
    outcome::result<SyncState> storeTipset(TipsetCPtr tipset,
                                           const TipsetKey &parent);

   private:
    /// Non-null error if state is inconsistent
    std::error_code state_error_;

    TsLoadPtr ts_load_;

    /// Index DB
    std::shared_ptr<IndexDb> index_db_;

    /// Genesis tipset
    TipsetCPtr genesis_tipset_;

    /// Graph of tipset branches
    Branches branches_;

    /// LRU cache of recently used tipsets
    TipsetCache tipset_cache_;

    /// Callback on head changes
    HeadCallback head_callback_;

    /// True if started
    bool started_ = false;
  };

}  // namespace fc::sync

OUTCOME_HPP_DECLARE_ERROR(fc::sync, ChainDb::Error);

#endif  // CPP_FILECOIN_SYNC_CHAIN_DB_HPP
