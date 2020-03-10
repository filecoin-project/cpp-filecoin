/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_BLOCKCHAIN_IMPL_SYNC_TARGET_BUCKET_HPP
#define CPP_FILECOIN_CORE_BLOCKCHAIN_IMPL_SYNC_TARGET_BUCKET_HPP

#include "primitives/tipset/tipset.hpp"

namespace fc::blockchain::sync_manager {

  /** @struct SyncTargetBucket stores bucket of tipset for synchronization */
  struct SyncTargetBucket {
    using Tipset = primitives::tipset::Tipset;
    std::vector<Tipset> tipsets;
    int count;

    /** @brief checks if tipset `ts` belongs to same chain */
    bool isSameChain(const Tipset &ts) const;

    /** @brief add tipset to the bucket */
    void addTipset(const Tipset &ts);

    /** @brief finds and returns heaviest tipset */
    Tipset getHeaviestTipset() const;
  };

  bool operator==(const SyncTargetBucket &lhs, const SyncTargetBucket &rhs);

}  // namespace fc::blockchain::sync_manager

#endif  // CPP_FILECOIN_CORE_BLOCKCHAIN_IMPL_SYNC_TARGET_BUCKET_HPP
