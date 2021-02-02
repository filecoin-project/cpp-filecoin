/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/tipset/chain.hpp"

#include "common/outcome2.hpp"

namespace fc::primitives::tipset::chain {
  TsBranchPtr TsBranch::make(TsChain chain, TsBranchPtr parent) {
    auto branch{std::make_shared<TsBranch>()};
    auto bottom{chain.begin()};
    branch->chain = std::move(chain);
    if (parent) {
      assert(*parent->chain.find(bottom->first) == *bottom);
      branch->parent = std::move(parent);
      branch->parent->children.push_back(branch);
    }
    return branch;
  }

  outcome::result<TsBranchPtr> TsBranch::make(TsLoadPtr ts_load,
                                              const TipsetKey &key,
                                              TsBranchPtr parent) {
    TsChain chain;
    OUTCOME_TRY(ts, ts_load->load(key));
    auto _parent{parent->chain.lower_bound(ts->height())};
    if (_parent == parent->chain.end()) {
      --_parent;
    }
    while (true) {
      auto _bottom{chain.emplace(ts->height(), TsLazy{ts->key, ts}).first};
      while (_parent->first > _bottom->first) {
        if (_parent == parent->chain.begin()) {
          return OutcomeError::kDefault;
        }
        --_parent;
      }
      if (*_parent == *_bottom) {
        break;
      }
      OUTCOME_TRYA(ts, ts_load->load(ts->getParents()));
    }
    return make(std::move(chain), parent);
  }

  outcome::result<TsBranchIter> find(TsBranchPtr branch,
                                     Height height,
                                     bool prev) {
    if (height > branch->chain.rbegin()->first) {
      return OutcomeError::kDefault;
    }
    TsBranchPtr parent;
    while (branch->chain.begin()->first > height) {
      if (!branch->parent) {
        return OutcomeError::kDefault;
      }
      parent = branch;
      branch = branch->parent;
    }
    auto it{branch->chain.lower_bound(height)};
    if (it == branch->chain.end()) {
      if (!prev) {
        // example: find 3 in [[1, 2], [4, 5]]
        if (!parent) {
          return OutcomeError::kDefault;
        }
        return std::make_pair(parent, parent->chain.begin());
      }
      --it;
    }
    if (it->first > height && prev) {
      if (it == branch->chain.begin()) {
        return OutcomeError::kDefault;
      }
      --it;
    }
    return std::make_pair(branch, it);
  }

  outcome::result<TsBranchIter> stepParent(TsBranchIter it) {
    auto &branch{it.first};
    if (it.second == branch->chain.begin()) {
      branch = branch->parent;
      if (!branch) {
        return OutcomeError::kDefault;
      }
      it.second = branch->chain.end();
    }
    --it.second;
    return it;
  }

  outcome::result<BeaconEntry> latestBeacon(TsLoadPtr ts_load,
                                            TsBranchPtr branch,
                                            Height height) {
    OUTCOME_TRY(it, find(branch, height));
    // magic number from lotus
    for (auto i{0}; i < 20; ++i) {
      OUTCOME_TRY(ts, ts_load->loadw(it.second->second));
      auto &beacons{ts->blks[0].beacon_entries};
      if (!beacons.empty()) {
        return *beacons.rbegin();
      }
      if (it.second->first == 0) {
        break;
      }
      OUTCOME_TRYA(it, stepParent(it));
    }
    return TipsetError::kNoBeacons;
  }
}  // namespace fc::primitives::tipset::chain
