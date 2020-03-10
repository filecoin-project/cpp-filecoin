/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_BLOCKCHAIN_IMPL_SYNC_BUCKET_SET_HPP
#define CPP_FILECOIN_CORE_BLOCKCHAIN_IMPL_SYNC_BUCKET_SET_HPP

#include "blockchain/impl/sync_target_bucket.hpp"

namespace fc::blockchain::sync_manager {
  /** @brief keeps and updates set of chains */
  class SyncBucketSet {
   public:
    using Tipset = primitives::tipset::Tipset;

    SyncBucketSet(gsl::span<const Tipset> tipsets);
    SyncBucketSet(std::vector<Tipset> tipsets);

    /** @brief checks if tipset is related to one of chains */
    bool isRelatedToAny(const Tipset &ts) const;

    /** @brief insert tipset */
    void insert(Tipset ts);

    /** @brief returns heaviest tipset and removes it from chain */
    outcome::result<SyncTargetBucket> pop();

    /** @brief remove bucket */
    void removeBucket(const SyncTargetBucket &b);

    /** @brief pops related tipset */
    boost::optional<SyncTargetBucket> popRelated(const Tipset &ts);

    /** @brief finds and returns heaviest tipset */
    boost::optional<Tipset> getHeaviestTipset() const;

    /** @brief checks if set is empty */
    bool isEmpty() const;

   private:
    std::vector<SyncTargetBucket> buckets;
  };
}  // namespace fc::blockchain::sync_manager

#endif  // CPP_FILECOIN_CORE_BLOCKCHAIN_IMPL_SYNC_BUCKET_SET_HPP
