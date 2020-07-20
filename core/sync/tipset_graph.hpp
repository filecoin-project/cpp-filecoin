/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipfs/datastore.hpp"
#include "branches.hpp"
#include "index_db.hpp"

#ifndef CPP_FILECOIN_SYNC_TIPSET_GRAPH_HPP
#define CPP_FILECOIN_SYNC_TIPSET_GRAPH_HPP

namespace fc::sync {

  using TipsetCache = LRUCache<TipsetHash, Tipset>;

  TipsetCache createTipsetCache(size_t max_size = 10000);

  using IpfsStoragePtr = std::shared_ptr<storage::ipfs::IpfsDatastore>;

  std::pair<KeyValueStoragePtr, IpfsStoragePtr> createStorages(
      const std::string &path);

  class ChainDb {
   public:
    enum Error {
      NOT_INITIALIZED = 1,
      GENESIS_MISMATCH,
      TIPSET_IS_BAD,
      TIPSET_NOT_FOUND,
    };

    ChainDb();

    outcome::result<void> init(KeyValueStoragePtr key_value_storage,
                               IpfsStoragePtr ipfs_storage,
                               std::shared_ptr<IndexDb> index_db,
                               const std::string &load_car_path);

    outcome::result<void> stateIsConsistent() const;

    const CID &genesisCID() const;

    const Tipset &genesisTipset() const;

    const std::string &networkName() const;

    bool tipsetIsStored(const TipsetHash &hash) const;

    outcome::result<void> getHeads(const HeadCallback &callback);

    outcome::result<TipsetCPtr> getTipsetByHash(const TipsetHash &hash);

    outcome::result<TipsetCPtr> getTipsetByHeight(Height height);

    using WalkCallback = std::function<void(TipsetCPtr tipset)>;

    outcome::result<void> walkForward(Height from_height,
                                      Height to_height,
                                      const WalkCallback &cb);

    outcome::result<void> walkBackward(const TipsetHash &from,
                                       Height to_height,
                                       const WalkCallback &cb);

    outcome::result<void> storeTipset(std::shared_ptr<Tipset> tipset,
                                      const TipsetHash &parent_hash,
                                      const HeadCallback &on_heads_changed);

   private:
    std::error_code state_error_;
    KeyValueStoragePtr key_value_storage_;
    IpfsStoragePtr ipfs_storage_;
    std::shared_ptr<IndexDb> index_db_;
    TipsetCPtr genesis_tipset_;
    std::string network_name_;
    Branches branches_;
    TipsetCache tipset_cache_;

  };

}  // namespace fc::sync

OUTCOME_HPP_DECLARE_ERROR(fc::sync, ChainDb::Error);

#endif  // CPP_FILECOIN_SYNC_TIPSET_GRAPH_HPP
