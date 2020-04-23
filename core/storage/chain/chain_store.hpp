/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_HPP
#define CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_HPP

#include "blockchain/weight_calculator.hpp"
#include "crypto/randomness/chain_randomness_provider.hpp"
#include "primitives/block/block.hpp"
#include "primitives/tipset/tipset.hpp"
#include "vm/message/message.hpp"

namespace fc::storage::blockchain {
  using crypto::randomness::ChainRandomnessProvider;
  using crypto::randomness::Randomness;
  using ::fc::blockchain::weight::WeightCalculator;
  using primitives::block::BlockHeader;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetKey;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;

  /**
   * @class ChainStore keeps track of blocks
   */
  class ChainStore : public ipfs::IpfsDatastore {
   public:
    struct Genesis {
      CID cid;
      Tipset tipset;
    };

    virtual ~ChainStore() = default;

    /**
     * @brief loads tipset from store
     * @param key tipset key
     */
    virtual outcome::result<Tipset> loadTipset(const TipsetKey &key) const = 0;

    /** @brief creates chain randomness provider */
    virtual std::shared_ptr<ChainRandomnessProvider>
    createRandomnessProvider() = 0;

    /** @brief adds block to store */
    virtual outcome::result<void> addBlock(const BlockHeader &block) = 0;

    /**@brief checks whether storage contains tipset */
    virtual outcome::result<bool> containsTipset(
        const TipsetKey &key) const = 0;

    /**@brief stores tipset */
    virtual outcome::result<void> storeTipset(const Tipset &tipset) = 0;

    virtual outcome::result<Tipset> heaviestTipset() const = 0;

    virtual outcome::result<BlockHeader> getGenesis() const = 0;

    virtual outcome::result<void> setGenesis(
        const BlockHeader &block_header) = 0;

    virtual primitives::BigInt getHeaviestWeight() const = 0;
  };

}  // namespace fc::storage::blockchain

#endif  // CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_HPP
