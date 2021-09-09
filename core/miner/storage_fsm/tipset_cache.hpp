/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/tipset/tipset.hpp"

namespace fc::mining {
  using primitives::ChainEpoch;
  using primitives::tipset::Tipset;

  class TipsetCache {
   public:
    virtual ~TipsetCache() = default;

    /**
     * Added tipset to cache
     * fill difference in height with boost::none
     * @param tipset is tipset that should be added
     * @return success or error
     */
    virtual outcome::result<void> add(const Tipset &tipset) = 0;

    /**
     * Revert tipset if it is head
     * @param tipset is tipset that should be reverted
     * @return success or error
     */
    virtual outcome::result<void> revert(const Tipset &tipset) = 0;

    /**
     * getNotNull tipset from this height
     * @param height is height of tipset that we want get
     * @return Tipset or error
     */
    virtual outcome::result<Tipset> getNonNull(ChainEpoch height) = 0;

    /**
     * get Tipset from cache with this height
     * @param height is height of tipset that we want get
     * @return optional Tipset or error
     */
    virtual outcome::result<boost::optional<Tipset>> get(ChainEpoch height) = 0;

    /**
     * Get Head tipset
     * @return head tipset
     */
    virtual boost::optional<Tipset> best() const = 0;
  };

  enum class TipsetCacheError {
    kSmallerHeight = 1,
    kNotMatchHead,
    kNotInCache,
  };
}  // namespace fc::mining

OUTCOME_HPP_DECLARE_ERROR(fc::mining, TipsetCacheError);
