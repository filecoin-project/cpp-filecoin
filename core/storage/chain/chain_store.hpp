/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_HPP
#define CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_HPP

#include <map>

#include "chain/block_validator.hpp"
#include "common/outcome.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/tipset/tipset.hpp"
#include "storage/ipfs/blockservice.hpp"
#include "storage/ipfs/datastore.hpp"

namespace fc::storage::chain {

  /**
   * @brief change type
   */
  enum class HeadChangeType { REVERT, APPLY, CURRENT };

  struct HeadChange {
    HeadChangeType type;
    std::reference_wrapper<const primitives::tipset::Tipset> value;
  };

  class ChainStore {
   public:
    /*
     * @brief creates new ChainStore instance
     */
    static outcome::result<ChainStore> create(
        std::shared_ptr<ipfs::BlockService> block_service,
        std::shared_ptr<ipfs::IpfsDatastore> data_store,
        std::shared_ptr<::fc::chain::block_validator::BlockValidator>
            block_validator);

    outcome::result<void> writeHead(const primitives::tipset::Tipset &tipset);

    /**
     * @brief loads data from block storage and initializes storage
     */
    outcome::result<void> load();

    /**
     * @brief loads tipset from store
     * @param key tipset key
     */
    outcome::result<primitives::tipset::Tipset> loadTipset(
        const primitives::tipset::TipsetKey &key);

    // TODO (yuraz): FIL-151 add notifications

    // TODO(yuraz): FIL-*** implement caching of items to avoid refetching them

   private:
    std::shared_ptr<ipfs::BlockService> block_service_;
    std::shared_ptr<ipfs::IpfsDatastore> data_store_;
    std::shared_ptr<::fc::chain::block_validator::BlockValidator>
        block_validator_;

    primitives::tipset::Tipset heaviest_tipset_;
    std::map<uint64_t, std::vector<CID>> tipsets_;
  };
}  // namespace fc::storage::chain

#endif  // CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_HPP
