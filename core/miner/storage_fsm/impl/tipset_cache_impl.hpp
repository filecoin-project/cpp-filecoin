/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MINER_STORAGE_FSM_IMPL_TIPSET_CACHE_IMPL_HPP
#define CPP_FILECOIN_CORE_MINER_STORAGE_FSM_IMPL_TIPSET_CACHE_IMPL_HPP

#include "miner/storage_fsm/tipset_cache.hpp"

namespace fc::mining {
  class TipsetCacheImpl : public TipsetCache {
   public:
    TipsetCacheImpl(uint64_t capability); // TODO: add download function

    outcome::result<void> add(const Tipset &tipset) override;

    outcome::result<void> revert(const Tipset &tipset) override;

    outcome::result<Tipset> getNonNull(uint64_t height) override;

    outcome::result<boost::optional<Tipset>> get(uint64_t height) override;

    boost::optional<Tipset> best() const override;

   private:
    std::vector<boost::optional<Tipset>> cache_;

    int64_t start_;

    uint64_t len_;
  };
}  // namespace fc::mining

#endif  // CPP_FILECOIN_CORE_MINER_STORAGE_FSM_IMPL_TIPSET_CACHE_IMPL_HPP
