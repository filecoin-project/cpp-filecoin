/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MINER_STORAGE_FSM_TIPSET_CACHE_HPP
#define CPP_FILECOIN_CORE_MINER_STORAGE_FSM_TIPSET_CACHE_HPP

namespace fc::mining {

  class TipsetCache {
   public:
    virtual ~TipsetCache() = default;

    virtual outcome::result<void> add(const Tipset &tipset) = 0;

    virtual outcome::result<void> revert(const Tipset &tipset) = 0;

    virtual outcome::result<Tipset> getNonNull(ChainEpoch height) const = 0;

    virtual outcome::result<boost::optional<Tipset>> get(
        ChainEpoch height) const = 0;

    virtual boost::optional<Tipset> best() const = 0;
  };

}  // namespace fc::mining

#endif  // CPP_FILECOIN_CORE_MINER_STORAGE_FSM_TIPSET_CACHE_HPP
