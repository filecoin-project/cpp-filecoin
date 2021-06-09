/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <map>
#include <mutex>
#include <libp2p/common/metrics/instance_count.hpp>

#include "cbor_blake/cid.hpp"
#include "primitives/tipset/tipset.hpp"

namespace fc::primitives::tipset {
  using CacheIndex = uint64_t;

  struct TsLazy {
    TipsetKey key;
    CacheIndex index{};

    LIBP2P_METRICS_INSTANCE_COUNT(fc::primitives::tipset::TsLazy);
  };
  inline auto operator==(const TsLazy &lhs, const TsLazy &rhs) {
    return lhs.key == rhs.key;
  }

  struct LoadCache {
    TipsetCPtr tipset;
    CacheIndex index{};

    LIBP2P_METRICS_INSTANCE_COUNT(fc::primitives::tipset::LoadCache);
  };

  struct LoadNode {
    uint64_t prev = 0;
    uint64_t next = 0;

    TipsetCPtr tipset = nullptr;

    LIBP2P_METRICS_INSTANCE_COUNT(fc::primitives::tipset::LoadNode);
  };

  struct TsLoad {
   public:
    virtual ~TsLoad() = default;
    virtual outcome::result<LoadCache> loadWithCacheInfo(
        const TipsetKey &key) = 0;
    virtual outcome::result<TipsetCPtr> load(const TipsetKey &key) = 0;
    virtual outcome::result<TipsetCPtr> load(std::vector<BlockHeader> blocks);
    virtual outcome::result<TipsetCPtr> lazyLoad(TsLazy &lazy) = 0;

    LIBP2P_METRICS_INSTANCE_COUNT(fc::primitives::tipset::TsLoad);
  };

  using TsLoadPtr = std::shared_ptr<TsLoad>;

  struct TsLoadIpld : public TsLoad {
   public:
    TsLoadIpld(IpldPtr ipld);
    outcome::result<LoadCache> loadWithCacheInfo(const TipsetKey &key) override;
    outcome::result<TipsetCPtr> load(const TipsetKey &key) override;
    outcome::result<TipsetCPtr> lazyLoad(TsLazy &lazy) override;

   private:
    IpldPtr ipld;

    LIBP2P_METRICS_INSTANCE_COUNT(fc::primitives::tipset::TsLoadIpld);
  };

  struct TsLoadCache : public TsLoad {
   public:
    TsLoadCache(TsLoadPtr ts_load, size_t cache_size);
    outcome::result<LoadCache> loadWithCacheInfo(const TipsetKey &key) override;
    outcome::result<TipsetCPtr> load(const TipsetKey &key) override;
    outcome::result<TipsetCPtr> load(std::vector<BlockHeader> blocks) override;
    outcome::result<TipsetCPtr> lazyLoad(TsLazy &lazy) override;

   private:
    outcome::result<TipsetCPtr> lazyLoad(uint64_t &cache_index,
                                         const TipsetKey &key);
    uint64_t cacheInsert(TipsetCPtr tipset);
    boost::optional<LoadCache> getFromCache(const TipsetKey &key);
    boost::optional<TipsetCPtr> getFromCache(uint64_t index,
                                             const TipsetKey &key);
    TipsetCPtr getFromCache(uint64_t index);

    TsLoadPtr ts_load;

    std::mutex mutex;
    std::map<TipsetKey, uint64_t> map_cache;
    std::vector<LoadNode> tipset_cache;
    uint64_t capacity;
    uint64_t begin_index{};
    uint64_t end_index{};

    LIBP2P_METRICS_INSTANCE_COUNT(fc::primitives::tipset::TsLoadCache);
  };

  struct PutBlockHeader {
    virtual ~PutBlockHeader() = default;
    virtual void put(const CbCid &key, BytesIn value) = 0;
  };
  CID put(const IpldPtr &ipld,
          const std::shared_ptr<PutBlockHeader> &put,
          const BlockHeader &header);
}  // namespace fc::primitives::tipset
