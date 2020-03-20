/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/sync_target_bucket.hpp"

namespace fc::blockchain::sync_manager {
  using Tipset = primitives::tipset::Tipset;

  outcome::result<bool> SyncTargetBucket::isSameChain(const Tipset &ts) const {
    for (auto &t : tipsets) {
      OUTCOME_TRY(ts_key, ts.makeKey());
      OUTCOME_TRY(t_key, t.makeKey());
      OUTCOME_TRY(t_parents_key, t.getParents());
      OUTCOME_TRY(ts_parents_key, t.getParents());

      if (t == ts) {
        return true;
      }

      if (ts_key == t_parents_key) {
        return true;
      }

      if (ts_parents_key == t_key) {
        return true;
      }
    }
    return false;
  }

  void SyncTargetBucket::addTipset(const Tipset &ts) {
    for (auto &t : tipsets) {
      if (t == ts) return;
    }
    tipsets.push_back(ts);
  }

  boost::optional<Tipset> SyncTargetBucket::getHeaviestTipset() const {
    boost::optional<Tipset> best_tipset;

    if (tipsets.empty()) {
      return boost::none;
    }

    for (auto &ts : tipsets) {
      if (best_tipset == boost::none
          || ts.getParentWeight() > best_tipset->getParentWeight()) {
        best_tipset = ts;
      }
    }

    return *best_tipset;
  }

  bool operator==(const SyncTargetBucket &lhs, const SyncTargetBucket &rhs) {
    return lhs.tipsets == rhs.tipsets;
  }

}  // namespace fc::blockchain::sync_manager

OUTCOME_CPP_DEFINE_CATEGORY(fc::blockchain::sync_manager,
                            SyncTargetBucketError,
                            error) {
  using Error = fc::blockchain::sync_manager::SyncTargetBucketError;
  switch (error) {
    case Error::BUCKET_IS_EMPTY:
      return "bucket is empty";
  }
}
