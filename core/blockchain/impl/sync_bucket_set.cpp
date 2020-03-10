/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/sync_bucket_set.hpp"

namespace fc::blockchain::sync_manager {
  using Tipset = primitives::tipset::Tipset;

  SyncBucketSet::SyncBucketSet(gsl::span<const Tipset> tipsets) {
    SyncTargetBucket b;
    b.tipsets.resize(tipsets.size());
    for (auto &t : tipsets) b.tipsets.push_back(t);
    buckets.push_back(std::move(b));
  }

  SyncBucketSet::SyncBucketSet(std::vector<Tipset> tipsets) {
    buckets.emplace_back(SyncTargetBucket{std::move(tipsets), 1});
  }

  bool SyncBucketSet::isRelatedToAny(const Tipset &ts) const {
    for (auto &b : buckets) {
      if (b.isSameChain(ts)) return true;
    }

    return false;
  }

  void SyncBucketSet::insert(Tipset ts) {
    for (auto &b : buckets)
      if (b.isSameChain(ts)) return b.addTipset(ts);

    buckets.emplace_back(SyncTargetBucket{{ts}, 1});
  }

  outcome::result<SyncTargetBucket> SyncBucketSet::pop() {
    boost::optional<SyncTargetBucket> best_bucket;
    boost::optional<Tipset> best_tipset;
    for (auto &b : buckets) {
      auto heaviest = b.getHeaviestTipset();
      if (!best_bucket.is_initialized()
          || best_tipset->getParentWeight() < heaviest.getParentWeight()) {
        best_bucket = b;
        best_tipset = heaviest;
      }
    }

    removeBucket(*best_bucket);
    return *best_bucket;
  }

  void SyncBucketSet::removeBucket(const SyncTargetBucket &b) {
    std::remove_if(buckets.begin(), buckets.end(), [&](const auto &item) {
      return item == b;
    });
  }

  boost::optional<SyncTargetBucket> SyncBucketSet::popRelated(
      const Tipset &ts) {
    for (auto &b : buckets) {
      if (b.isSameChain(ts)) {
        auto bucket = b;
        removeBucket(b);
        return bucket;
      }
    }
    return boost::none;
  }

  boost::optional<Tipset> SyncBucketSet::getHeaviestTipset() const {
    boost::optional<Tipset> heaviest;
    for (auto &b : buckets) {
      auto ts = b.getHeaviestTipset();
      if (heaviest == boost::none
          || ts.getParentWeight() > heaviest->getParentWeight())
        heaviest = ts;
    }

    return heaviest;
  }

  bool SyncBucketSet::isEmpty() const {
    return buckets.size() > 0;
  }

}  // namespace fc::blockchain::sync_manager
