/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_HPP
#define CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_HPP

#include "crypto/randomness/chain_randomness_provider.hpp"
#include "primitives/block/block.hpp"
#include "primitives/tipset/tipset.hpp"

namespace fc::storage::blockchain {

  using crypto::randomness::ChainRandomnessProvider;
  using crypto::randomness::Randomness;
  using primitives::block::BlockHeader;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetKey;

  /**
   * @class ChainStore keeps track of blocks
   */
  class ChainStore {
   public:
    virtual ~ChainStore() = default;

    /**
     * @brief loads tipset from store
     * @param key tipset key
     */
    virtual outcome::result<Tipset> loadTipset(const TipsetKey &key) = 0;

    inline outcome::result<Tipset> loadParent(const Tipset &tipset) {
      return loadTipset(tipset.blks[0].parents);
    }

    /** @brief creates chain randomness provider */
    virtual std::shared_ptr<ChainRandomnessProvider>
    createRandomnessProvider() = 0;

    /** @brief adds block to store */
    virtual outcome::result<void> addBlock(const BlockHeader &block) = 0;

    /** @brief finds block by its cid */
    virtual outcome::result<BlockHeader> getBlock(const CID &cid) const = 0;

    virtual outcome::result<Tipset> heaviestTipset() const = 0;
  };

}  // namespace fc::storage::blockchain

#endif  // CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_HPP
