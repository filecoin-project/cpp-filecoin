/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_BLOCKCHAIN_IMPL_SYNC_BUCKET_SET_HPP
#define CPP_FILECOIN_CORE_BLOCKCHAIN_IMPL_SYNC_BUCKET_SET_HPP

#include "blockchain/impl/sync_target_bucket.hpp"

namespace fc::blockchain::sync_manager {
  class SyncBucketSet {
   public:
    using Tipset = primitives::tipset::Tipset;
    SyncBucketSet(gsl::span<const Tipset> tipsets) {
      SyncTargetBucket b;
      b.tipsets.reset(tipsets.size());
      std::copy(tipsets.begin(), tipsets.end(), b.tipsets.begin());
      return {b};
    }

    bool isRelatedToAny(const Tipset &ts) {
      for (auto &b : buckets) {
        if (b.isSameChain(ts)) return true;
      }

      return false;
    }

    void insert(Tipset ts) {
      for (auto &b : buckets) {
        if (b.isSameChain(ts)) return b.add(ts);
      }

      buckets.push_back(SyncBucketSet({ts}));
    }

    SyncTargetBucket pop() {
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

      removeBucket(best_bucket);
      return best_bucket;
    }

    void removeBucket(const SyncTargetBucket &b) {
      std::remove_if(buckets.begin(), buckets.end(), [&](const auto &item) {
        return item == b;
      });
    }

    boost::optional<SyncTargetBucket> popRelated(const Tipset ts) {
      for (auto &b : buckets) {
        if (b.isSameChain(ts)) {
          auto bucket = b;
          removeBucket(b);
          return bucket;
        }

        return boost::none;
      }
    }

    boost::optional<Tipset> getHeaviestTipset() {
      boost::optional<Tipset> heaviest;
      for (auto &b: buckets) {
        ts = b.getHeaviestTipset();
        if (heaviest == boost::none || ts.getParentWeight() > heaviest->getParentWeight())
          heaviest = ts;
      }

      return heaviest;
    }

    bool isEmpty() {
      return buckets.size() > 0;
    }

   private:
    std::vector<SyncTargetBucket> buckets;
  };
}  // namespace fc::blockchain::sync_manager

#endif  // CPP_FILECOIN_CORE_BLOCKCHAIN_IMPL_SYNC_BUCKET_SET_HPP
