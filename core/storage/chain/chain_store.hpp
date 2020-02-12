/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_HPP
#define CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_HPP

#include <map>

#include "chain/block_validator.hpp"
#include "common/logger.hpp"
#include "common/outcome.hpp"
#include "crypto/randomness/randomness_types.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/tipset/tipset.hpp"
#include "storage/chain/chain_data_store.hpp"
#include "storage/ipfs/blockservice.hpp"

namespace fc::storage::chain {

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
    using BlockValidator = ::fc::chain::block_validator::BlockValidator;
    using Tipset = primitives::tipset::Tipset;
    using TipsetKey = primitives::tipset::TipsetKey;
    using Randomness = crypto::randomness::Randomness;
    /*
     * @brief creates new ChainStore instance
     */
    static outcome::result<ChainStore> create(
        std::shared_ptr<ipfs::BlockService> block_service,
        std::shared_ptr<ChainDataStore> data_store,
        std::shared_ptr<BlockValidator> block_validator);

    outcome::result<void> writeHead(const Tipset &tipset);

    /**
     * @brief loads data from block storage and initializes storage
     */
    outcome::result<void> load();

    /**
     * @brief loads tipset from store
     * @param key tipset key
     */
    outcome::result<Tipset> loadTipset(const TipsetKey &key);

    // TODO (yuraz): FIL-151 add notifications

    // TODO(yuraz): FIL-151 implement items caching to avoid refetching them

    /** draws randomness */
    outcome::result<Randomness> drawRandomness(const std::vector<CID> &blks,
                                               uint64_t round);

   private:
    ChainStore(std::shared_ptr<ipfs::BlockService> block_service,
               std::shared_ptr<ChainDataStore> data_store,
               std::shared_ptr<BlockValidator> block_validator);

    void takeHeaviestTipSet(const Tipset &tipset);

    std::shared_ptr<ipfs::BlockService> block_service_;
    std::shared_ptr<ChainDataStore> data_store_;
    std::shared_ptr<BlockValidator> block_validator_;

    boost::optional<Tipset> heaviest_tipset_;
    std::map<uint64_t, std::vector<CID>> tipsets_;

    common::Logger logger_;
  };
}  // namespace fc::storage::chain

OUTCOME_HPP_DECLARE_ERROR(fc::storage::chain, ChainStoreError);

#endif  // CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_HPP
