/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_HPP
#define CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_HPP

#include <boost/signals2.hpp>
#include "blockchain/weight_calculator.hpp"
#include "primitives/block/block.hpp"
//#include "primitives/cid/cid_of_cbor.hpp"
#include "primitives/tipset/tipset.hpp"
#include "vm/message/message.hpp"

namespace fc::storage::blockchain {
  using primitives::block::BlockHeader;
  using primitives::tipset::HeadChange;
  using primitives::tipset::HeadChangeType;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetKey;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;

  /// represents chain path between 2 tipsets in a tree
  struct ChainPath {
    std::deque<Tipset> revert_chain;  ///< tipsets to revert
    std::deque<Tipset> apply_chain;   ///< tipsets to apply
  };

  using primitives::block::BlockHeader;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetKey;

  enum class ChainStoreError : int {
    NO_MIN_TICKET_BLOCK = 1,
    NO_HEAVIEST_TIPSET,
    NO_GENESIS_BLOCK,
    STORE_NOT_INITIALIZED,
    DATA_INTEGRITY_ERROR,
  };

  class ChainStore {
   public:
    virtual ~ChainStore() = default;

    virtual outcome::result<void> init(BlockHeader genesis_header) = 0;

    virtual outcome::result<Tipset> loadTipset(const TipsetKey &key) = 0;

    virtual outcome::result<void> addBlock(const BlockHeader &block) = 0;
//
//    //virtual outcome::result<bool> containsTipset(
//    //    const TipsetKey &key) const = 0;
//
    virtual outcome::result<Tipset> heaviestTipset() const = 0;

    virtual outcome::result<BlockHeader> getGenesis() const = 0;

//    virtual outcome::result<void> writeGenesis(
//        const BlockHeader &block_header) = 0;

//    //virtual primitives::BigInt getHeaviestWeight() const = 0;

    using connection_t = boost::signals2::connection;
    using HeadChangeSignature = void(const HeadChange &);

    virtual connection_t subscribeHeadChanges(
        const std::function<HeadChangeSignature> &subscriber) = 0;

    virtual outcome::result<TipsetKey> genesisTipsetKey() const = 0;
    virtual outcome::result<CID> genesisCID() const = 0;

  };

}  // namespace fc::storage::blockchain

OUTCOME_HPP_DECLARE_ERROR(fc::storage::blockchain, ChainStoreError);

#endif  // CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_HPP
