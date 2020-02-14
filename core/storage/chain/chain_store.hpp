/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_HPP
#define CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_HPP

#include <map>

#include "blockchain/block_validator.hpp"
#include "blockchain/weight_calculator.hpp"
#include "common/logger.hpp"
#include "common/outcome.hpp"
#include "crypto/randomness/randomness_types.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/tipset/tipset.hpp"
#include "storage/chain/chain_data_store.hpp"
#include "storage/ipfs/blockservice.hpp"

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
   * @struct MmCids is a struct for using in mmCache cache
   */
  struct MmCids {
    std::vector<CID> bls;
    std::vector<CID> secpk;
  };

  enum class ChainStoreError : int { NO_MIN_TICKET_BLOCK = 1 };

  /**
   * @class ChainStore keeps track of blocks
   */
  class ChainStore {
   public:
    using BlockValidator = ::fc::blockchain::block_validator::BlockValidator;
    using Tipset = primitives::tipset::Tipset;
    using TipsetKey = primitives::tipset::TipsetKey;
    using Randomness = crypto::randomness::Randomness;
    using BlockHeader = primitives::block::BlockHeader;
    using WeightCalculator = ::fc::blockchain::weight::WeightCalculator;

    /* @brief creates new ChainStore instance */
    static outcome::result<std::shared_ptr<ChainStore>> create(
        std::shared_ptr<ipfs::BlockService> block_service,
        std::shared_ptr<ChainDataStore> data_store,
        std::shared_ptr<BlockValidator> block_validator,
        std::shared_ptr<WeightCalculator> weight_calculator);

    /** @brief stores head tipset */
    outcome::result<void> writeHead(const Tipset &tipset);

    /** @brief loads data from block storage and initializes storage */
    outcome::result<void> load();

    /**
     * @brief loads tipset from store
     * @param key tipset key
     */
    outcome::result<Tipset> loadTipset(const TipsetKey &key);

    // TODO(yuraz): FIL-151 add notifications
    // TODO(yuraz): FIL-151 implement items caching to avoid refetching them

    /** @brief draws randomness */
    outcome::result<Randomness> sampleRandomness(const std::vector<CID> &blks,
                                                 uint64_t round);

    /** @brief finds block by its cid */
    outcome::result<BlockHeader> getBlock(const CID &cid) const;

    /** @brief adds block to store */
    outcome::result<void> addBlock(const BlockHeader &block);

   private:
    ChainStore(std::shared_ptr<ipfs::BlockService> block_service,
               std::shared_ptr<ChainDataStore> data_store,
               std::shared_ptr<BlockValidator> block_validator,
               std::shared_ptr<WeightCalculator> weight_calculator);

    ChainStore(ChainStore &&other) = default;

    outcome::result<void> takeHeaviestTipset(const Tipset &tipset);

    outcome::result<void> persistBlockHeaders(
        const std::vector<std::reference_wrapper<const BlockHeader>>
            &block_headers);

    outcome::result<Tipset> expandTipset(const BlockHeader &block_header);

    outcome::result<void> updateHeavierTipset(const Tipset &tipset);

    std::shared_ptr<ipfs::BlockService> block_service_;
    std::shared_ptr<ChainDataStore> data_store_;
    std::shared_ptr<BlockValidator> block_validator_;
    std::shared_ptr<WeightCalculator> weight_calculator_;

    boost::optional<Tipset> heaviest_tipset_;
    std::map<uint64_t, std::vector<CID>> tipsets_;

    common::Logger logger_;
  };
}  // namespace fc::storage::blockchain

OUTCOME_HPP_DECLARE_ERROR(fc::storage::blockchain, ChainStoreError);

#endif  // CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_HPP
