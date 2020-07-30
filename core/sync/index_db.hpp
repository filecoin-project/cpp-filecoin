/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_INDEX_DB_HPP
#define CPP_FILECOIN_SYNC_INDEX_DB_HPP

#include "common.hpp"
#include "lru_cache.hpp"
#include "storage/buffer_map.hpp"

namespace fc::sync {

  struct TipsetInfo {
    TipsetKey key;

    BranchId branch = kNoBranch;
    Height height = 0;
    TipsetHash parent_hash;
  };

  using TipsetInfoPtr = std::shared_ptr<TipsetInfo>;
  using TipsetInfoCPtr = std::shared_ptr<const TipsetInfo>;

  using KeyValueStoragePtr = std::shared_ptr<storage::PersistentBufferMap>;

  class IndexDbBackend;

  class IndexDb {
   public:
    explicit IndexDb(/*KeyValueStoragePtr kv_store,*/
            std::shared_ptr<IndexDbBackend> backend);

    outcome::result<std::map<BranchId, std::shared_ptr<BranchInfo>>> init();

    outcome::result<void> storeGenesis(const Tipset& genesis_tipset);

    outcome::result<void> store(
        TipsetInfoPtr info, const boost::optional<RenameBranch> &branch_rename);

    bool contains(const TipsetHash &hash);

    outcome::result<TipsetInfoCPtr> get(const TipsetHash &hash);

    outcome::result<TipsetInfoCPtr> get(BranchId branch, Height height);

    using WalkCallback = std::function<void(TipsetInfoCPtr info)>;

    outcome::result<void> walkForward(BranchId branch,
                                      Height from_height,
                                      Height to_height,
                                      const WalkCallback &cb);

    outcome::result<void> walkBackward(const TipsetHash &from,
                                       Height to_height,
                                       const WalkCallback &cb);

   private:
    using Cache = LRUCache<TipsetHash, TipsetInfo>;

//    KeyValueStoragePtr kv_store_;
    std::shared_ptr<IndexDbBackend> backend_;
    Cache cache_;
  };

}  // namespace fc::sync

#endif  // CPP_FILECOIN_SYNC_INDEX_DB_HPP
