/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_BLOCKCHAIN_IMPL_SYNC_BUCKET_SET_HPP
#define CPP_FILECOIN_CORE_BLOCKCHAIN_IMPL_SYNC_BUCKET_SET_HPP

#include "blockchain/impl/sync_target_bucket.hpp"
#include "common/outcome.hpp"

namespace fc::blockchain::sync_manager {

  enum class SyncBucketSetError { BUCKET_NOT_FOUND = 1 };

  /** @brief keeps and updates set of chains */
  class SyncBucketSet {
   public:
    using Tipset = primitives::tipset::Tipset;

    /** @brief constructors */
    explicit SyncBucketSet(gsl::span<const Tipset> tipsets);
    explicit SyncBucketSet(std::vector<Tipset> tipsets);

    /** @brief checks if tipset is related to one of chains */
    outcome::result<bool> isRelatedToAny(const Tipset &ts) const;

    /** @brief insert tipset */
    void insert(Tipset ts);

    /** @brief appends bucket */
    void append(SyncTargetBucket bucket);

    /** @brief returns heaviest tipset and removes it from chain */
    boost::optional<SyncTargetBucket> pop();

    /** @brief remove bucket */
    void removeBucket(const SyncTargetBucket &b);

    /** @brief pops related tipset */
    outcome::result<boost::optional<SyncTargetBucket>> popRelated(
        const Tipset &ts);

    /** @brief finds and returns heaviest tipset */
    outcome::result<Tipset> getHeaviestTipset() const;

    /** @brief checks if set is empty */
    bool isEmpty() const;

    /** @brief returns buckets count */
    size_t getSize() const;

   protected:
    std::vector<SyncTargetBucket> buckets_;
  };
}  // namespace fc::blockchain::sync_manager

OUTCOME_HPP_DECLARE_ERROR(fc::blockchain::sync_manager, SyncBucketSetError);

#endif  // CPP_FILECOIN_CORE_BLOCKCHAIN_IMPL_SYNC_BUCKET_SET_HPP
