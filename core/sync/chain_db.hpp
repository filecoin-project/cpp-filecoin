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

  class ChainDb {
   public:
    /// Heads configuration changed callback. If both values are present then
    /// it means that 'added' replaces 'removed'
    using HeadCallback = std::function<void(std::vector<TipsetHash> removed,
                                            std::vector<TipsetHash> added)>;

    ChainDb();

    outcome::result<void> init(IpfsStoragePtr ipld,
                               std::shared_ptr<IndexDb> index_db,
                               const boost::optional<CID> &genesis_cid,
                               bool creating_new_db);

    outcome::result<void> start(HeadCallback on_heads_changed);

    outcome::result<void> stateIsConsistent() const;

    const CID &genesisCID() const;

    const Tipset &genesisTipset() const;

    bool tipsetIsStored(const TipsetHash &hash) const;

    outcome::result<void> getHeads(const HeadCallback &callback);

    outcome::result<TipsetCPtr> getTipsetByHash(const TipsetHash &hash);

    outcome::result<TipsetCPtr> getTipsetByHeight(Height height);

    outcome::result<TipsetCPtr> getTipsetByKey(const TipsetKey &key);

    outcome::result<TipsetCPtr> findHighestCommonAncestor(const TipsetCPtr &a,
                                                          const TipsetCPtr &b);

    outcome::result<void> setCurrentHead(const TipsetHash &head);

    using WalkCallback = std::function<void(TipsetCPtr tipset)>;

    outcome::result<void> walkForward(Height from_height,
                                      Height to_height,
                                      const WalkCallback &cb);

    outcome::result<void> walkBackward(const TipsetHash &from,
                                       Height to_height,
                                       const WalkCallback &cb);

    /// returns next unsynced tipset to be loaded, if any
    outcome::result<boost::optional<TipsetCPtr>> storeTipset(
        TipsetCPtr tipset, const TipsetKey &parent);

    outcome::result<boost::optional<TipsetCPtr>> getUnsyncedBottom(
        const TipsetKey &key);

   private:
    outcome::result<TipsetCPtr> loadTipsetFromIpld(const TipsetKey &key);

    std::error_code state_error_;
    // KeyValueStoragePtr key_value_storage_;
    IpfsStoragePtr ipld_;
    std::shared_ptr<IndexDb> index_db_;
    TipsetCPtr genesis_tipset_;
    Branches branches_;
    TipsetCache tipset_cache_;
    HeadCallback head_callback_;
    bool started_ = false;
  };

}  // namespace fc::sync

#endif  // CPP_FILECOIN_SYNC_CHAIN_DB_HPP
