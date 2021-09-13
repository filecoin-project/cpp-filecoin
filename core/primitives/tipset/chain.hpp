/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/ptr.hpp"
#include "primitives/tipset/load.hpp"
#include "storage/buffer_map.hpp"

namespace fc::primitives::tipset::chain::file {
  struct Updater;
}  // namespace fc::primitives::tipset::chain::file

namespace fc::primitives::tipset::chain {
  using TsChain = std::map<ChainEpoch, TsLazy>;

  struct TsBranch;
  using TsBranchPtr = std::shared_ptr<TsBranch>;
  using TsBranchWeak = std::weak_ptr<TsBranch>;
  using TsBranchIter = std::pair<TsBranchPtr, TsChain::iterator>;
  using TsBranchChildren = std::multimap<ChainEpoch, TsBranchWeak>;

  struct TsBranch {
    /**
     * @return parent if chain size is 1 else valid branch attached to parent
     */
    static TsBranchPtr make(TsChain chain, TsBranchPtr parent = nullptr);
    static outcome::result<TsBranchPtr> make(TsLoadPtr ts_load,
                                             const TipsetKey &key,
                                             TsBranchPtr parent);

    TsChain::value_type &bottom();
    /// load to height if lazy
    void lazyLoad(ChainEpoch height);

    struct Lazy {
      TsChain::value_type bottom;
      size_t min_load{100};
    };

    TsChain chain;
    TsBranchPtr parent;
    TsBranchChildren children;
    boost::optional<TipsetKey> parent_key;
    boost::optional<Lazy> lazy;
    std::shared_ptr<file::Updater> updater;
  };

  /**
   * inclusive revert and apply chains
   */
  using Path = std::pair<TsChain, TsChain>;

  outcome::result<std::pair<Path, std::vector<TsBranchPtr>>> update(
      TsBranchPtr branch, TsBranchIter to_it);

  using TsBranches = std::set<TsBranchPtr>;
  using TsBranchesPtr = std::shared_ptr<TsBranches>;
  TsBranchIter find(const TsBranches &branches, TipsetCPtr ts);
  TsBranchIter insert(TsBranches &branches,
                      TipsetCPtr ts,
                      std::vector<TsBranchPtr> *children = {});

  std::vector<TsBranchIter> children(TsBranchIter ts_it);

  /**
   * @return valid iterator
   */
  outcome::result<TsBranchIter> find(TsBranchPtr branch,
                                     ChainEpoch height,
                                     bool allow_less = true);

  outcome::result<TsBranchIter> stepParent(TsBranchIter it);

  outcome::result<BeaconEntry> latestBeacon(TsLoadPtr ts_load, TsBranchIter it);

  outcome::result<TsBranchIter> getLookbackTipSetForRound(TsBranchIter it,
                                                          ChainEpoch epoch);
}  // namespace fc::primitives::tipset::chain
