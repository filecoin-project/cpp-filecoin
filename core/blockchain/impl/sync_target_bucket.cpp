/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/sync_target_bucket.hpp"

namespace fc::blockchain::sync_manager {
  using Tipset = primitives::tipset::Tipset;

  bool SyncTargetBucket::isSameChain(const Tipset &ts) const {
    for (auto &t : tipsets) {
      if (t == ts || ts.makeKey() == t.getParents()
          || ts.getParents() == t.makeKey())
        return true;
    }
    return false;
  }

  void SyncTargetBucket::addTipset(const Tipset &ts) {
    for (auto &t : tipsets) {
      if (t == ts) return;
    }
    ++count;
    tipsets.push_back(ts);
  }

  Tipset SyncTargetBucket::getHeaviestTipset() const {
    boost::optional<Tipset> best_tipset;

    BOOST_ASSERT_MSG(tipsets.empty(), "tipsets count == 0");
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
