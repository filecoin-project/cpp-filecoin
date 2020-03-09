/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_HPP
#define CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_HPP

#include "filecoin/primitives/block/block.hpp"
#include "filecoin/primitives/tipset/tipset.hpp"

namespace fc::storage::blockchain {

  /**
   * @brief change type
   */
  enum class HeadChangeType : int { REVERT, APPLY, CURRENT };

  /**
   * @struct HeadChange represents atomic chain change
   */
  struct HeadChange {
    HeadChangeType type;
    primitives::tipset::Tipset value;
  };

  /**
   * @class ChainStore keeps track of blocks
   */
  class ChainStore {
   public:
    using BlockHeader = primitives::block::BlockHeader;
    using ChainRandomnessProvider = crypto::randomness::ChainRandomnessProvider;
    using Randomness = crypto::randomness::Randomness;
    using Tipset = primitives::tipset::Tipset;
    using TipsetKey = primitives::tipset::TipsetKey;

    virtual ~ChainStore() = default;

    /**
     * @brief loads tipset from store
     * @param key tipset key
     */
    virtual outcome::result<Tipset> loadTipset(const TipsetKey &key) = 0;

    /** @brief creates chain randomness provider */
    virtual std::shared_ptr<ChainRandomnessProvider>
    createRandomnessProvider() = 0;

    /** @brief adds block to store */
    virtual outcome::result<void> addBlock(const BlockHeader &block) = 0;

    /** @brief finds block by its cid */
    virtual outcome::result<BlockHeader> getBlock(const CID &cid) const = 0;
  };

}  // namespace fc::storage::blockchain

#endif  // CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_HPP
