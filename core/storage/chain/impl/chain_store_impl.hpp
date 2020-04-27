/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_IMPL_HPP
#define CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_IMPL_HPP

#include <map>

#include "blockchain/block_validator/block_validator.hpp"
#include "blockchain/weight_calculator.hpp"
#include "common/logger.hpp"
#include "common/outcome.hpp"
#include "crypto/randomness/chain_randomness_provider.hpp"
#include "crypto/randomness/randomness_types.hpp"
#include "primitives/cid/cid.hpp"
#include "storage/chain/chain_data_store.hpp"
#include "storage/chain/chain_store.hpp"
#include "storage/ipfs/impl/ipfs_block_service.hpp"

namespace fc::storage::blockchain {

  using ::fc::blockchain::block_validator::BlockValidator;
  using ::fc::blockchain::weight::WeightCalculator;
  using ipfs::IpfsDatastore;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetKey;

  /** @brief chain store errors enum */
  enum class ChainStoreError : int {
    NO_MIN_TICKET_BLOCK = 1,
    NO_HEAVIEST_TIPSET,
    NO_GENESIS_BLOCK,
    STORE_NOT_INITIALIZED,
  };

  class ChainStoreImpl : public ChainStore,
                         public std::enable_shared_from_this<ChainStoreImpl> {
   public:
    ChainStoreImpl(ChainStoreImpl &&other) = default;

    /* @brief creates new ChainStore instance */
    static outcome::result<std::shared_ptr<ChainStoreImpl>> create(
        std::shared_ptr<IpfsDatastore> data_store,
        std::shared_ptr<BlockValidator> block_validator,
        std::shared_ptr<WeightCalculator> weight_calculator);

    /** @brief stores head tipset */
    outcome::result<void> writeHead(const Tipset &tipset);

    /** @brief loads data from block storage and initializes storage */
    outcome::result<void> initialize();

    outcome::result<Tipset> loadTipset(const TipsetKey &key) const override;

    outcome::result<void> addBlock(const BlockHeader &block) override;

    outcome::result<Tipset> heaviestTipset() const override;

    outcome::result<bool> containsTipset(const TipsetKey &key) const override;

    outcome::result<void> storeTipset(const Tipset &tipset) override;

    outcome::result<BlockHeader> getGenesis() const override;

    outcome::result<void> writeGenesis(
        const BlockHeader &block_header) override;

    primitives::BigInt getHeaviestWeight() const override {
      return heaviest_weight_;
    }

    /** IpfsDatastore implementation is required for using in ChainDownloader */
    outcome::result<bool> contains(const CID &key) const override {
      return data_store_->contains(key);
    }

    outcome::result<void> set(const CID &key, Value value) override {
      return data_store_->set(key, value);
    }

    outcome::result<Value> get(const CID &key) const override {
      return data_store_->get(key);
    }

    outcome::result<void> remove(const CID &key) override {
      return data_store_->remove(key);
    }

    /** chain randomness */
    std::shared_ptr<ChainRandomnessProvider> createRandomnessProvider()
        override;

    /** @brief head change subscription */
    connection_t subscribeHeadChanges(
        const std::function<HeadChangeSignature> &subscriber) override {
      return head_change_signal_.connect(subscriber);
    }

   private:
    ChainStoreImpl(std::shared_ptr<IpfsDatastore> data_store,
                   std::shared_ptr<BlockValidator> block_validator,
                   std::shared_ptr<WeightCalculator> weight_calculator);

    outcome::result<void> takeHeaviestTipset(const Tipset &tipset);

    outcome::result<void> persistBlockHeaders(
        const std::vector<std::reference_wrapper<const BlockHeader>>
            &block_headers);

    outcome::result<Tipset> expandTipset(const BlockHeader &block_header);

    outcome::result<void> updateHeaviestTipset(const Tipset &tipset);

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
    ///< wrapper around main data storage to store tipset keys
    std::shared_ptr<ChainDataStore> chain_data_store_;
    std::shared_ptr<BlockValidator> block_validator_;
    std::shared_ptr<WeightCalculator> weight_calculator_;

    boost::optional<Tipset> heaviest_tipset_;  ///< current heaviest tipset
    primitives::BigInt heaviest_weight_{0};    ///< current heaviest weight
    boost::optional<BlockHeader> genesis_;     ///< genesis block
    std::unordered_map<uint64_t, std::vector<CID>> tipsets_;
    mutable std::unordered_map<TipsetKey, Tipset> tipsets_cache_;

    ///< when head tipset changes, need to notify all subscribers
    boost::signals2::signal<HeadChangeSignature> head_change_signal_;

    common::Logger logger_;
  };
}  // namespace fc::storage::blockchain

OUTCOME_HPP_DECLARE_ERROR(fc::storage::blockchain, ChainStoreError);

#endif  // CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_IMPL_HPP
