/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_IMPL_HPP
#define CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_IMPL_HPP

#include <map>

#include "blockchain/block_validator.hpp"
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

  /**
   * @struct MmCids is a struct for using in mmCache cache
   */
  struct MmCids {
    std::vector<CID> bls;
    std::vector<CID> secpk;
  };

  enum class ChainStoreError : int { NO_MIN_TICKET_BLOCK = 1 };

  class ChainStoreImpl : public ChainStore,
                         public std::enable_shared_from_this<ChainStoreImpl> {
   public:
    using BlockValidator = ::fc::blockchain::block_validator::BlockValidator;
    using WeightCalculator = ::fc::blockchain::weight::WeightCalculator;

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

    outcome::result<Tipset> loadTipset(const TipsetKey &key) override;

    // TODO(yuraz): FIL-151 add notifications
    // TODO(yuraz): FIL-151 implement items caching to avoid refetching them

    std::shared_ptr<ChainRandomnessProvider> createRandomnessProvider()
        override;

    outcome::result<void> addBlock(const BlockHeader &block) override;

    outcome::result<BlockHeader> getBlock(const CID &cid) const override;

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

    outcome::result<void> updateHeavierTipset(const Tipset &tipset);

    std::shared_ptr<ipfs::IpfsBlockService> block_service_;
    std::shared_ptr<ChainDataStore> data_store_;
    std::shared_ptr<BlockValidator> block_validator_;
    std::shared_ptr<WeightCalculator> weight_calculator_;

    boost::optional<Tipset> heaviest_tipset_;
    std::map<uint64_t, std::vector<CID>> tipsets_;

    common::Logger logger_;
  };
}  // namespace fc::storage::blockchain

OUTCOME_HPP_DECLARE_ERROR(fc::storage::blockchain, ChainStoreError);

#endif  // CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_IMPL_HPP
