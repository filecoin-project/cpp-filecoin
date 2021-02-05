/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/tipset/load.hpp"

namespace fc::primitives::tipset::chain {

  using TsChain = std::map<Height, TsLazy>;

  struct TsBranch;
  using TsBranchPtr = std::shared_ptr<TsBranch>;
  using TsBranchWeak = std::weak_ptr<TsBranch>;
  using TsBranchIter = std::pair<TsBranchPtr, TsChain::iterator>;

  struct TsBranch {
    /**
     * @return parent if chain size is 1 else valid branch attached to parent
     */
    static TsBranchPtr make(TsChain chain, TsBranchPtr parent = nullptr);
    static outcome::result<TsBranchPtr> make(TsLoadPtr ts_load,
                                             const TipsetKey &key,
                                             TsBranchPtr parent);

    TsChain chain;
    TsBranchPtr parent;
    std::vector<TsBranchWeak> children;
  };

  /**
   * inclusive revert and apply chains
   */
  using Path = std::pair<TsChain, TsChain>;
  /**
   * @param to_it valid iterator of valid branch
   */
  outcome::result<Path> findPath(TsBranchPtr from, TsBranchIter to_it);

  /**
   * @return valid iterator
   */
  outcome::result<TsBranchIter> find(TsBranchPtr branch,
                                     Height height,
                                     bool allow_less = true);

  outcome::result<BeaconEntry> latestBeacon(TsLoadPtr ts_load,
                                            TsBranchPtr branch,
                                            Height height);
}  // namespace fc::primitives::tipset::chain
