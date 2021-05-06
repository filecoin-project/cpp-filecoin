/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/tipset/load.hpp"

namespace fc::primitives::tipset {
  outcome::result<TipsetCPtr> TsLoad::loadw(TsLazy &lazy) {
    return loadw(lazy.cache_index, lazy.key);
  }
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

  outcome::result<LoadCache> TsLoadIpld::loadc(const TipsetKey &key) {
    OUTCOME_TRY(ts, load(key));
    return LoadCache{.tipset = ts, .cache_index = 0};  // we don't have cache
  }

  outcome::result<TipsetCPtr> TsLoadIpld::loadw(uint64_t &cache_index,
                                                const TipsetKey &key) {
    return load(key);  // we don't have cache
  }

  outcome::result<TipsetCPtr> TsLoadIpld::loadw(TsLazy &lazy) {
    return TsLoad::loadw(lazy);
  }

  TsLoadCache::TsLoadCache(TsLoadPtr ts_load, size_t cache_size)
      : ts_load{std::move(ts_load)}, capacity{cache_size} {}

  outcome::result<TipsetCPtr> TsLoadCache::load(const TipsetKey &key) {
    OUTCOME_TRY(ts, loadc(key));

    return std::move(ts.tipset);
  }

  outcome::result<TipsetCPtr> TsLoadCache::load(
      std::vector<BlockHeader> blocks) {
    OUTCOME_TRY(ts, ts_load->load(std::move(blocks)));

    cacheInsert(ts);

    return std::move(ts);
  }

  outcome::result<TipsetCPtr> TsLoadCache::loadw(uint64_t &cache_index,
                                                 const TipsetKey &key) {
    if (auto tipset{getFromCache(cache_index, key)}) {
      return tipset.get();
    }

    OUTCOME_TRY(ts, loadc(key));
    cache_index = ts.cache_index;
    return std::move(ts.tipset);
  }

  uint64_t TsLoadCache::cacheInsert(TipsetCPtr tipset) {
    std::lock_guard lock{mutex};

    auto res = map_cache.insert(std::make_pair(tipset->key, 0));

    if (tipset_cache.empty()) {
      begin_index = 0;
      end_index = 1;
      auto tcn{std::make_shared<LoadNode>()};
      tcn->tipset = std::move(tipset);
      tcn->prev = begin_index;
      tcn->next = begin_index;
      tcn->it = res.first;
      tcn->it->second = begin_index;
      tipset_cache.push_back(tcn);
      return begin_index;
    }

    if (tipset_cache.size() < capacity) {
      tipset_cache[end_index - 1]->next = end_index;

      auto tcn{std::make_shared<LoadNode>()};
      tcn->tipset = std::move(tipset);
      tcn->prev = end_index - 1;
      tcn->next = end_index;
      tcn->it = res.first;
      tcn->it->second = end_index;
      tipset_cache.push_back(tcn);
      return end_index++;
    }

    uint64_t new_index = end_index;
    auto new_tipset{tipset_cache[new_index]};
    map_cache.erase(new_tipset->it);
    new_tipset->it = res.first;
    new_tipset->it->second = new_index;

    tipset_cache[new_tipset->prev]->next = new_tipset->prev;
    end_index = new_tipset->prev;

    tipset_cache[begin_index]->prev = new_index;
    new_tipset->prev = new_index;
    new_tipset->next = begin_index;
    new_tipset->tipset = tipset;

    begin_index = new_index;
    return new_index;
  }

  outcome::result<TipsetCPtr> TsLoadCache::loadw(TsLazy &lazy) {
    return TsLoad::loadw(lazy);
  }

  TipsetCPtr TsLoadCache::getFromCache(uint64_t index) {
    auto ts{tipset_cache[index]};
    // when it first
    if (index == begin_index) {
      return ts->tipset;
    }

    // when it last
    if (end_index == index) {
      tipset_cache[ts->prev]->next = ts->prev;
      end_index = ts->prev;
    } else {
      // when it medium
      tipset_cache[ts->next]->prev = ts->prev;
      tipset_cache[ts->prev]->next = ts->next;
    }

    tipset_cache[begin_index]->prev = index;
    ts->prev = index;
    ts->next = begin_index;

    begin_index = index;
    return ts->tipset;
  }

  boost::optional<TipsetCPtr> TsLoadCache::getFromCache(uint64_t index,
                                                        const TipsetKey &key) {
    std::lock_guard lock{mutex};

    if (key != tipset_cache[index]->tipset->key) {
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

  outcome::result<LoadCache> TsLoadCache::loadc(const TipsetKey &key) {
    if (auto ts{getFromCache(key)}) {
      return ts.value();
    }

    OUTCOME_TRY(ts, ts_load->load(key));

    return LoadCache{.tipset = ts, .cache_index = cacheInsert(ts)};
  }
}  // namespace fc::primitives::tipset
