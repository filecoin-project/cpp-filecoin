/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_HPP
#define CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_HPP

#include <boost/signals2.hpp>
#include "primitives/tipset/tipset.hpp"

namespace fc::storage::blockchain {
  using primitives::block::BlockHeader;
  using primitives::tipset::HeadChange;
  using primitives::tipset::HeadChangeType;
  using primitives::tipset::TipsetCPtr;
  using primitives::tipset::TipsetKey;
  using primitives::tipset::TipsetHash;

  enum class ChainStoreError : int {
    kStoreNotInitialized = 1,
    kNoHeaviestTipset,
    kNoTipsetAtHeight,
    kBlockRejected,
    kIllegalState,
    //    NO_MIN_TICKET_BLOCK,

//    NO_GENESIS_BLOCK,
//    STORE_NOT_INITIALIZED,
//    DATA_INTEGRITY_ERROR,
  };

  class ChainStore {
   public:
    virtual ~ChainStore() = default;

    virtual outcome::result<void> start() = 0;

    virtual outcome::result<void> addBlock(const BlockHeader &block) = 0;

    virtual outcome::result<TipsetCPtr> loadTipset(const TipsetHash &hash) = 0;

    virtual outcome::result<TipsetCPtr> loadTipset(const TipsetKey &key) = 0;

    virtual outcome::result<TipsetCPtr> loadTipsetByHeight(uint64_t height) = 0;

    virtual outcome::result<TipsetCPtr> heaviestTipset() const = 0;

    using connection_t = boost::signals2::connection;
    using HeadChangeSignature = void(const HeadChange &);

    virtual connection_t subscribeHeadChanges(
        const std::function<HeadChangeSignature> &subscriber) = 0;

    virtual const std::string& getNetworkName() const = 0;

    virtual const CID& genesisCID() const = 0;
  };

}  // namespace fc::storage::blockchain

OUTCOME_HPP_DECLARE_ERROR(fc::storage::blockchain, ChainStoreError);

#endif  // CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_HPP
