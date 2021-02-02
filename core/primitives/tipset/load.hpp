/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/compute/detail/lru_cache.hpp>

#include "primitives/tipset/tipset.hpp"

namespace fc::primitives::tipset {
  struct TsLazy {
    TipsetKey key;
    TsWeak weak{};
  };
  inline auto operator==(const TsLazy &lhs, const TsLazy &rhs) {
    return lhs.key == rhs.key;
  }

  struct TsLoad {
    virtual ~TsLoad() = default;
    virtual outcome::result<TipsetCPtr> load(const TipsetKey &key) = 0;
    virtual outcome::result<TipsetCPtr> load(std::vector<BlockHeader> blocks);
    outcome::result<TipsetCPtr> load(TsWeak &weak, const TipsetKey &key);
    outcome::result<TipsetCPtr> load(TsLazy &lazy);
  };
  using TsLoadPtr = std::shared_ptr<TsLoad>;

  struct TsLoadIpld : TsLoad {
    TsLoadIpld(IpldPtr ipld);
    outcome::result<TipsetCPtr> load(const TipsetKey &key) override;
    outcome::result<TipsetCPtr> load(std::vector<BlockHeader> blocks) override;
    IpldPtr ipld;
  };

  struct TsLoadCache : TsLoad {
    TsLoadCache(TsLoadPtr loader, size_t cache_size);
    outcome::result<TipsetCPtr> load(const TipsetKey &key) override;
    outcome::result<TipsetCPtr> load(std::vector<BlockHeader> blocks) override;
    TsLoadPtr loader;
    boost::compute::detail::lru_cache<TipsetKey, TipsetCPtr> cache;
  };
}  // namespace fc::primitives::tipset
