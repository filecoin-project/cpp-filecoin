/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_BLOCKCHAIN_IMPL_SYNC_TARGET_BUCKET_HPP
#define CPP_FILECOIN_CORE_BLOCKCHAIN_IMPL_SYNC_TARGET_BUCKET_HPP

#include "blockchain/impl/sync_target_bucket.hpp"

#include "common/outcome.hpp"
#include "primitives/block/block.hpp"
#include "primitives/tipset/tipset.hpp"

namespace fc::blockchain::sync_manager {

  enum class SyncTargetBucketError : int {
    BUCKET_IS_EMPTY = 1,
  };

  /** @struct SyncTargetBucket stores bucket of tipsets for synchronization */
  struct SyncTargetBucket {
    using Tipset = primitives::tipset::Tipset;
    std::vector<Tipset> tipsets;

    /** @brief returns tipsets count */
    size_t getSize() const {
      return tipsets.size();
    }

    /** @brief checks if tipset `ts` belongs to same chain */
    outcome::result<bool> isSameChain(const Tipset &ts) const;

    /** @brief add tipset to the bucket */
    void addTipset(const Tipset &ts);

    /** @brief finds and returns heaviest tipset */
    boost::optional<Tipset> getHeaviestTipset() const;
  };

  bool operator==(const SyncTargetBucket &lhs, const SyncTargetBucket &rhs);

}  // namespace fc::blockchain::sync_manager

OUTCOME_HPP_DECLARE_ERROR(fc::blockchain::sync_manager, SyncTargetBucketError);

#endif  // CPP_FILECOIN_CORE_BLOCKCHAIN_IMPL_SYNC_TARGET_BUCKET_HPP
