/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/sync_bucket_set.hpp"

namespace fc::blockchain::sync_manager {
  using Tipset = primitives::tipset::Tipset;

  SyncBucketSet::SyncBucketSet(gsl::span<const Tipset> tipsets) {
    if (tipsets.empty()) {
      return;
    }

    SyncTargetBucket b;
    b.tipsets.resize(tipsets.size());
    for (auto &t : tipsets) {
      b.tipsets.push_back(t);
    }
    buckets_.push_back(std::move(b));
  }

  SyncBucketSet::SyncBucketSet(std::vector<Tipset> tipsets) {
    if (!tipsets.empty()) {
      buckets_.emplace_back(SyncTargetBucket{std::move(tipsets)});
    }
  }

  outcome::result<bool> SyncBucketSet::isRelatedToAny(const Tipset &ts) const {
    for (auto &b : buckets_) {
      OUTCOME_TRY(res, b.isSameChain(ts));
      if (res) {
        return true;
      }
    }

    return false;
  }

  void SyncBucketSet::insert(Tipset ts) {
    for (auto &b : buckets_)
      if (b.isSameChain(ts)) {
        return b.addTipset(ts);
      }

    buckets_.emplace_back(SyncTargetBucket{{ts}});
  }

  void SyncBucketSet::append(SyncTargetBucket bucket) {
    buckets_.push_back(std::move(bucket));
  }

  boost::optional<SyncTargetBucket> SyncBucketSet::pop() {
    boost::optional<SyncTargetBucket> best_bucket;
    boost::optional<Tipset> best_tipset;
    for (auto &b : buckets_) {
      auto heaviest = b.getHeaviestTipset();
      if (boost::none == heaviest) {
        continue;
      }

      if (!best_bucket.is_initialized()
          || best_tipset->getParentWeight()
                 < heaviest->getParentWeight()) {
        best_bucket = b;
        best_tipset = heaviest;
      }
    }

    if (boost::none != best_bucket) {
      removeBucket(*best_bucket);
    }

    return best_bucket;
  }

  void SyncBucketSet::removeBucket(const SyncTargetBucket &b) {
    auto end = std::remove_if(buckets_.begin(),
                              buckets_.end(),
                              [&](const auto &item) { return item == b; });
    buckets_.erase(end, buckets_.end());
  }

  outcome::result<boost::optional<SyncTargetBucket>> SyncBucketSet::popRelated(
      const Tipset &ts) {
    for (auto &b : buckets_) {
      OUTCOME_TRY(res, b.isSameChain(ts));
      if (!res) {
        continue;
      }

      auto bucket = b;
      removeBucket(b);

      return bucket;
    }

    return boost::none;
  }

  outcome::result<Tipset> SyncBucketSet::getHeaviestTipset() const {
    boost::optional<Tipset> heaviest;
    for (auto &b : buckets_) {
      auto ts = b.getHeaviestTipset();
      if (boost::none == ts) {
        continue;
      }

      if (heaviest == boost::none
          || ts->getParentWeight() > heaviest->getParentWeight())
        heaviest = ts;
    }

    if (heaviest) {
      return *heaviest;
    }

    return SyncBucketSetError::BUCKET_NOT_FOUND;
  }

  bool SyncBucketSet::isEmpty() const {
    return buckets_.empty();
  }

  size_t SyncBucketSet::getSize() const {
    return buckets_.size();
  }

}  // namespace fc::blockchain::sync_manager

OUTCOME_CPP_DEFINE_CATEGORY(fc::blockchain::sync_manager,
                            SyncBucketSetError,
                            error) {
  using Error = fc::blockchain::sync_manager::SyncBucketSetError;
  switch (error) {
    case Error::BUCKET_NOT_FOUND:
      return "bucket not found";
  }
}
