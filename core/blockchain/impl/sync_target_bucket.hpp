/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_BLOCKCHAIN_IMPL_SYNC_TARGET_BUCKET_HPP
#define CPP_FILECOIN_CORE_BLOCKCHAIN_IMPL_SYNC_TARGET_BUCKET_HPP

#include "primitives/tipset/tipset.hpp"

namespace fc::blockchain::sync_manager {

  struct SyncTargetBucket {
    using Tipset = primitives::tipset::Tipset;
    std::vector<Tipset> tipsets;
    int count;

    bool isSameChain(const Tipset &ts) const {
      for (auto &t : tipsets) {
        if (t == ts || ts.makeKey() == t.getParents()
            || ts.getParents() == t.makeKey())
          return true;
      }
      return false;
    }

    void addTipset(const Tipset &ts) {
      for (auto &t : tipsets) {
        if (t == ts) return;
      }
      ++count;
      tipsets.push_back(ts);
    }

    Tipset getHeaviestTipset() {
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
  };

}  // namespace fc::blockchain::sync_manager

#endif  // CPP_FILECOIN_CORE_BLOCKCHAIN_IMPL_SYNC_TARGET_BUCKET_HPP
