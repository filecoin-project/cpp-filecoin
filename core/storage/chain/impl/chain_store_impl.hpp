/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_IMPL_HPP
#define CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_IMPL_HPP

#include <map>

#include "blockchain/weight_calculator.hpp"
#include "common/logger.hpp"
#include "storage/chain/chain_store.hpp"

namespace fc::storage::blockchain {
  using ::fc::blockchain::weight::WeightCalculator;
  using ipfs::IpfsDatastore;

  enum class ChainStoreError { kNoPath = 1 };

  class ChainStoreImpl : public ChainStore {
   public:
    ChainStoreImpl(IpldPtr ipld,
                   std::shared_ptr<WeightCalculator> weight_calculator,
                   BlockHeader genesis,
                   Tipset head);

    outcome::result<void> addBlock(const BlockHeader &block) override;

    const Tipset &heaviestTipset() const override;

    const BlockHeader &getGenesis() const override;

    primitives::BigInt getHeaviestWeight() const override {
      return heaviest_weight_;
    }

    /** @brief head change subscription */
    connection_t subscribeHeadChanges(
        const std::function<HeadChangeSignature> &subscriber) override {
      subscriber(HeadChange{.type = HeadChangeType::CURRENT,
                            .value = heaviest_tipset_});
      return head_change_signal_.connect(subscriber);
    }

    outcome::result<void> updateHeaviestTipset(const Tipset &tipset) override;

   private:
    /**
     * @brief applies new heaviest tipset if better than old item
     * @param tipset new heaviest tipset
     */
    outcome::result<void> takeHeaviestTipset(const Tipset &tipset);

    /** @brief finds and returns tipset containing given block header */
    outcome::result<Tipset> expandTipset(const BlockHeader &block_header);

    /**
     * @brief finds path from current tipset to new tipset
     * @param current denotes current tipset
     * @param target denotes new tipset
     * @return path from current to target tipset
     */
    outcome::result<ChainPath> findChainPath(const Tipset &current,
                                             const Tipset &target);

    /**
     * @brief notifies all head change subscribers
     * @param current denotes current tipset
     * @param target denotes target tipset
     */
    outcome::result<void> notifyHeadChange(const Tipset &current,
                                           const Tipset &target);

    ///< main data storage
    std::shared_ptr<IpfsDatastore> data_store_;
    std::shared_ptr<WeightCalculator> weight_calculator_;

    Tipset heaviest_tipset_;                 ///< current heaviest tipset
    primitives::BigInt heaviest_weight_{0};  ///< current heaviest weight
    BlockHeader genesis_;                    ///< genesis block
    std::unordered_map<uint64_t, std::vector<CID>> tipsets_;

    ///< when head tipset changes, need to notify all subscribers
    boost::signals2::signal<HeadChangeSignature> head_change_signal_;

    common::Logger logger_{common::createLogger("chain store")};
  };
}  // namespace fc::storage::blockchain

OUTCOME_HPP_DECLARE_ERROR(fc::storage::blockchain, ChainStoreError);

#endif  // CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_IMPL_HPP
