/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <shared_mutex>
#include "api/full_node/node_api.hpp"
#include "miner/storage_fsm/tipset_cache.hpp"

namespace fc::mining {
  using api::FullNodeApi;

  class TipsetCacheImpl : public TipsetCache {
   public:
    using GetTipsetFunction =
        std::function<outcome::result<primitives::tipset::TipsetCPtr>(
            ChainEpoch)>;

    TipsetCacheImpl(uint64_t capability, std::shared_ptr<FullNodeApi> api);

    outcome::result<void> add(const Tipset &tipset) override;

    outcome::result<void> revert(const Tipset &tipset) override;

    outcome::result<Tipset> getNonNull(ChainEpoch height) override;

    outcome::result<boost::optional<Tipset>> get(ChainEpoch height) override;

    outcome::result<Tipset> best() const override;

   private:
    int64_t mod(int64_t x);

    mutable std::shared_mutex mutex_;

    std::vector<boost::optional<Tipset>> cache_;

    int64_t start_;

    uint64_t len_;

    GetTipsetFunction get_function_;

    std::shared_ptr<FullNodeApi> api_;
  };
}  // namespace fc::mining
