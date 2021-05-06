/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <map>
#include <mutex>

#include "primitives/tipset/tipset.hpp"

namespace fc::primitives::tipset {
  struct TsLazy {
    TipsetKey key;
    uint64_t cache_index;
  };
  inline auto operator==(const TsLazy &lhs, const TsLazy &rhs) {
    return lhs.key == rhs.key;
  }

  struct LoadCache {
    TipsetCPtr tipset;
    uint64_t cache_index = 0;
  };

  struct LoadNode {
    uint64_t prev = 0;
    uint64_t next = 0;

    std::map<TipsetKey, uint64_t>::iterator it;

    TipsetCPtr tipset = nullptr;
  };

  struct TsLoad {
   public:
    virtual ~TsLoad() = default;
    virtual outcome::result<LoadCache> loadc(const TipsetKey &key) = 0;
    virtual outcome::result<TipsetCPtr> load(const TipsetKey &key) = 0;
    virtual outcome::result<TipsetCPtr> load(std::vector<BlockHeader> blocks);
    virtual outcome::result<TipsetCPtr> loadw(uint64_t &cache_index,
                                              const TipsetKey &key) = 0;
    virtual outcome::result<TipsetCPtr> loadw(TsLazy &lazy);
  };

  using TsLoadPtr = std::shared_ptr<TsLoad>;

  struct TsLoadIpld : public TsLoad {
   public:
    TsLoadIpld(IpldPtr ipld);
    outcome::result<LoadCache> loadc(const TipsetKey &key) override;
    outcome::result<TipsetCPtr> load(const TipsetKey &key) override;
    outcome::result<TipsetCPtr> load(std::vector<BlockHeader> blocks) override;
    outcome::result<TipsetCPtr> loadw(uint64_t &cache_index,
                                      const TipsetKey &key) override;
    outcome::result<TipsetCPtr> loadw(TsLazy &lazy) override;

   private:
    IpldPtr ipld;
  };

  struct TsLoadCache : public TsLoad {
   public:
    TsLoadCache(TsLoadPtr ts_load, size_t cache_size);
    outcome::result<LoadCache> loadc(const TipsetKey &key) override;
    outcome::result<TipsetCPtr> load(const TipsetKey &key) override;
    outcome::result<TipsetCPtr> load(std::vector<BlockHeader> blocks) override;
    outcome::result<TipsetCPtr> loadw(uint64_t &cache_index,
                                      const TipsetKey &key) override;
    outcome::result<TipsetCPtr> loadw(TsLazy &lazy) override;

   private:
    uint64_t cacheInsert(TipsetCPtr tipset);
    boost::optional<LoadCache> getFromCache(const TipsetKey &key);
    boost::optional<TipsetCPtr> getFromCache(uint64_t index,
                                             const TipsetKey &key);
    TipsetCPtr getFromCache(uint64_t index);

    TsLoadPtr ts_load;

    std::mutex mutex;
    std::map<TipsetKey, uint64_t> map_cache;
    std::vector<std::shared_ptr<LoadNode>> tipset_cache;
    uint64_t capacity;
    uint64_t begin_index;
    uint64_t end_index;
  };
}  // namespace fc::primitives::tipset
