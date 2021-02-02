/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/tipset/load.hpp"

namespace fc::primitives::tipset {
  outcome::result<TipsetCPtr> TsLoad::load(TsWeak &weak, const TipsetKey &key) {
    if (auto ts{weak.lock()}) {
      return ts;
    }
    weak.reset();
    OUTCOME_TRY(ts, load(key));
    weak = ts;
    return std::move(ts);
  }

  outcome::result<TipsetCPtr> TsLoad::load(TsLazy &lazy) {
    return load(lazy.weak, lazy.key);
  }

  outcome::result<TipsetCPtr> TsLoad::load(std::vector<BlockHeader> blocks) {
    return Tipset::create(std::move(blocks));
  }

  TsLoadIpld::TsLoadIpld(IpldPtr ipld) : ipld{ipld} {}

  outcome::result<TipsetCPtr> TsLoadIpld::load(const TipsetKey &key) {
    std::vector<BlockHeader> blocks;
    blocks.reserve(key.cids().size());
    for (auto &cid : key.cids()) {
      OUTCOME_TRY(block, ipld->getCbor<BlockHeader>(cid));
      blocks.emplace_back(std::move(block));
    }
    return TsLoad::load(blocks);
  }

  outcome::result<TipsetCPtr> TsLoadIpld::load(
      std::vector<BlockHeader> blocks) {
    for (auto &block : blocks) {
      OUTCOME_TRY(ipld->setCbor(block));
    }
    return TsLoad::load(std::move(blocks));
  }

  TsLoadCache::TsLoadCache(TsLoadPtr loader, size_t cache_size)
      : loader{loader}, cache{cache_size} {}

  outcome::result<TipsetCPtr> TsLoadCache::load(const TipsetKey &key) {
    if (auto ts{cache.get(key)}) {
      return *ts;
    }
    OUTCOME_TRY(ts, loader->load(key));
    cache.insert(key, ts);
    return std::move(ts);
  }

  outcome::result<TipsetCPtr> TsLoadCache::load(
      std::vector<BlockHeader> blocks) {
    OUTCOME_TRY(ts, loader->load(blocks));
    cache.insert(ts->key, ts);
    return std::move(ts);
  }
}  // namespace fc::primitives::tipset
