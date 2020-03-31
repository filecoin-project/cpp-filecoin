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
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetKey;
  /**
   * @struct MmCids is a struct for using in mmCache cache
   */
  struct MmCids {
    std::vector<CID> bls;
    std::vector<CID> secpk;
  };

  enum class ChainStoreError : int {
    NO_MIN_TICKET_BLOCK = 1,
    NO_HEAVIEST_TIPSET,
    STORE_NOT_INITIALIZED,
  };

  class ChainStoreImpl : public ChainStore,
                         public std::enable_shared_from_this<ChainStoreImpl> {
   public:
    ChainStoreImpl(ChainStoreImpl &&other) = default;

    /* @brief creates new ChainStore instance */
    static outcome::result<std::shared_ptr<ChainStoreImpl>> create(
        std::shared_ptr<ipfs::IpfsBlockService> block_service,
        std::shared_ptr<ChainDataStore> data_store,
        std::shared_ptr<BlockValidator> block_validator,
        std::shared_ptr<WeightCalculator> weight_calculator);

    /** @brief stores head tipset */
    outcome::result<void> writeHead(const Tipset &tipset);

    /** @brief loads data from block storage and initializes storage */
    outcome::result<void> load();

    outcome::result<Tipset> loadTipset(const TipsetKey &key) const override;

    // TODO(yuraz): FIL-151 add notifications
    // TODO(yuraz): FIL-151 implement items caching to avoid refetching them

    std::shared_ptr<ChainRandomnessProvider> createRandomnessProvider()
        override;

    outcome::result<void> addBlock(const BlockHeader &block) override;

    outcome::result<BlockHeader> getBlock(const CID &cid) const override;

    outcome::result<Tipset> heaviestTipset() const override;

    outcome::result<bool> containsTipset(const TipsetKey &key) const override;

    outcome::result<void> storeTipset(const Tipset &tipset) override;

    outcome::result<SignedMessage> getSignedMessage(
        const CID &cid) const override;

    outcome::result<UnsignedMessage> getUnsignedMessage(
        const CID &cid) const override;

    const CID &getGenesis() const override {
      BOOST_ASSERT_MSG(genesis_.has_value(), "genesis is not initialized");
      return *genesis_;
    }

    primitives::BigInt getHeaviestWeight() const override {
      return heaviest_weight_;
    }

    /** IpfsDatastore implementation is required for using in ChainDownloader
     * refactore ChainStore later, remove this ugly inheritance and delegation
     */
    outcome::result<bool> contains(const CID &key) const override {
      return block_service_->contains(key);
    }

    outcome::result<void> set(const CID &key, Value value) override {
      return block_service_->set(key, value);
    }

    outcome::result<Value> get(const CID &key) const override {
      return block_service_->get(key);
    }

    outcome::result<void> remove(const CID &key) override {
      return block_service_->remove(key);
    }

   private:
    ChainStoreImpl(std::shared_ptr<ipfs::IpfsBlockService> block_service,
                   std::shared_ptr<ChainDataStore> data_store,
                   std::shared_ptr<BlockValidator> block_validator,
                   std::shared_ptr<WeightCalculator> weight_calculator);

    outcome::result<void> takeHeaviestTipset(const Tipset &tipset);

    outcome::result<void> persistBlockHeaders(
        const std::vector<std::reference_wrapper<const BlockHeader>>
            &block_headers);

    outcome::result<Tipset> expandTipset(const BlockHeader &block_header);

    outcome::result<void> updateHeaviestTipset(const Tipset &tipset);

    std::shared_ptr<ipfs::IpfsBlockService> block_service_;
    std::shared_ptr<ChainDataStore> data_store_;
    std::shared_ptr<BlockValidator> block_validator_;
    std::shared_ptr<WeightCalculator> weight_calculator_;

    boost::optional<Tipset> heaviest_tipset_;
    primitives::BigInt heaviest_weight_{0};
    std::map<uint64_t, std::vector<CID>> tipsets_;
    boost::optional<CID> genesis_;

    mutable std::unordered_map<TipsetKey, Tipset> tipsets_cache_;
    common::Logger logger_;
  };
}  // namespace fc::storage::blockchain

OUTCOME_HPP_DECLARE_ERROR(fc::storage::blockchain, ChainStoreError);

#endif  // CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_IMPL_HPP
