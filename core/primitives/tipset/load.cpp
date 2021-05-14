/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/tipset/load.hpp"

namespace fc::primitives::tipset {

  outcome::result<TipsetCPtr> TsLoad::load(std::vector<BlockHeader> blocks) {
    return Tipset::create(std::move(blocks));
  }

  TsLoadIpld::TsLoadIpld(IpldPtr ipld) : ipld{std::move(ipld)} {}

  outcome::result<TipsetCPtr> TsLoadIpld::load(const TipsetKey &key) {
    std::vector<BlockHeader> blocks;
    blocks.reserve(key.cids().size());
    for (auto &cid : key.cids()) {
      OUTCOME_TRY(block, ipld->getCbor<BlockHeader>(cid));
      blocks.emplace_back(std::move(block));
    }
    return TsLoad::load(std::move(blocks));
  }

  outcome::result<TipsetCPtr> TsLoadIpld::load(
      std::vector<BlockHeader> blocks) {
    for (auto &block : blocks) {
      OUTCOME_TRY(ipld->setCbor(block));
    }
    return TsLoad::load(std::move(blocks));
  }

  outcome::result<LoadCache> TsLoadIpld::loadWithCacheInfo(const TipsetKey &key) {
    OUTCOME_TRY(ts, load(key));
    return LoadCache{.tipset = ts, .index = 0};  // we don't have cache
  }

  outcome::result<TipsetCPtr> TsLoadIpld::lazyLoad(TsLazy &lazy) {
    return load(lazy.key);  // we don't have cache
  }

  TsLoadCache::TsLoadCache(TsLoadPtr ts_load, size_t cache_size)
      : ts_load{std::move(ts_load)}, capacity{cache_size} {}

  outcome::result<TipsetCPtr> TsLoadCache::load(const TipsetKey &key) {
    OUTCOME_TRY(ts, loadWithCacheInfo(key));

    return std::move(ts.tipset);
  }

  outcome::result<TipsetCPtr> TsLoadCache::load(
      std::vector<BlockHeader> blocks) {
    OUTCOME_TRY(ts, ts_load->load(std::move(blocks)));

    cacheInsert(ts);

    return std::move(ts);
  }

  outcome::result<TipsetCPtr> TsLoadCache::lazyLoad(uint64_t &cache_index,
                                                 const TipsetKey &key) {
    if (auto tipset{getFromCache(cache_index, key)}) {
      return tipset.get();
    }

    OUTCOME_TRY(ts, loadWithCacheInfo(key));
    cache_index = ts.index;
    return std::move(ts.tipset);
  }

  uint64_t TsLoadCache::cacheInsert(TipsetCPtr tipset) {
    std::lock_guard lock{mutex};

    auto res = map_cache.insert(std::make_pair(tipset->key, 0));

    if (tipset_cache.empty()) {
      begin_index = 0;
      end_index = 0;
      LoadNode tcn{
          .prev = begin_index,
          .next = begin_index,
          .it = res.first,
          .tipset = std::move(tipset),
      };
      tcn.it->second = begin_index;
      tipset_cache.push_back(tcn);
      return begin_index;
    }

    if (tipset_cache.size() < capacity) {
      begin_index += 1;
      tipset_cache[begin_index - 1].prev = begin_index;
      LoadNode tcn{
          .prev = begin_index,
          .next = begin_index - 1,
          .it = res.first,
          .tipset = std::move(tipset),
      };
      tcn.it->second = begin_index;
      tipset_cache.push_back(tcn);
      return begin_index;
    }

    uint64_t new_index = end_index;
    auto& new_tipset{tipset_cache[new_index]};
    map_cache.erase(new_tipset.it);
    new_tipset.it = res.first;
    new_tipset.it->second = new_index;

    tipset_cache[new_tipset.prev].next = new_tipset.prev;
    end_index = new_tipset.prev;

    tipset_cache[begin_index].prev = new_index;
    new_tipset.prev = new_index;
    new_tipset.next = begin_index;
    new_tipset.tipset = tipset;

    begin_index = new_index;
    return new_index;
  }

  outcome::result<TipsetCPtr> TsLoadCache::lazyLoad(TsLazy &lazy) {
    return lazyLoad(lazy.index, lazy.key);
  }

  TipsetCPtr TsLoadCache::getFromCache(uint64_t index) {
    auto ts{tipset_cache[index]};
    // when it first
    if (index == begin_index) {
      return ts.tipset;
    }

    // when it last
    if (end_index == index) {
      tipset_cache[ts.prev].next = ts.prev;
      end_index = ts.prev;
    } else {
      // when it medium
      tipset_cache[ts.next].prev = ts.prev;
      tipset_cache[ts.prev].next = ts.next;
    }

    tipset_cache[begin_index].prev = index;
    ts.prev = index;
    ts.next = begin_index;

    begin_index = index;
    return ts.tipset;
  }

  boost::optional<TipsetCPtr> TsLoadCache::getFromCache(uint64_t index,
                                                        const TipsetKey &key) {
    std::lock_guard lock{mutex};

    if (key != tipset_cache[index].tipset->key) {
      return boost::none;
    }

    return getFromCache(index);
  }

  boost::optional<LoadCache> TsLoadCache::getFromCache(const TipsetKey &key) {
    std::lock_guard lock{mutex};
    auto it{map_cache.find(key)};

    if (it == map_cache.end()) {
      return boost::none;
    }

    return LoadCache{getFromCache(it->second), it->second};
  }

  outcome::result<LoadCache> TsLoadCache::loadWithCacheInfo(const TipsetKey &key) {
    if (auto ts{getFromCache(key)}) {
      return ts.value();
    }

    OUTCOME_TRY(ts, ts_load->load(key));

    return LoadCache{.tipset = ts, .index = cacheInsert(ts)};
  }
}  // namespace fc::primitives::tipset
