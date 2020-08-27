/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_HPP
#define CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_HPP

#include <boost/signals2.hpp>
#include "blockchain/weight_calculator.hpp"
#include "primitives/block/block.hpp"
#include "primitives/cid/cid_of_cbor.hpp"
#include "primitives/tipset/tipset.hpp"
#include "vm/message/message.hpp"

namespace fc::storage::blockchain {
  using ::fc::blockchain::weight::WeightCalculator;
  using primitives::block::BlockHeader;
  using primitives::tipset::HeadChange;
  using primitives::tipset::HeadChangeType;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetKey;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;

  /**
   * @struct ChainPath represents chain path between 2 tipsets in a tree
   */
  struct ChainPath {
    std::deque<Tipset> revert_chain;  ///< tipsets to revert
    std::deque<Tipset> apply_chain;   ///< tipsets to apply
  };

  /**
   * @class ChainStore keeps track of blocks
   */
  class ChainStore {
   public:
    virtual ~ChainStore() = default;

    /** @brief adds block to storage */
    virtual outcome::result<void> addBlock(const BlockHeader &block) = 0;

    /** @brief returns current heaviest tipset */
    virtual const Tipset &heaviestTipset() const = 0;

    /** @brief loads genesis block from storage */
    virtual const BlockHeader &getGenesis() const = 0;

    /** @brief returns heaviest tipset weight, 0 if not set */
    virtual primitives::BigInt getHeaviestWeight() const = 0;

    using connection_t = boost::signals2::connection;
    using HeadChangeSignature = void(const HeadChange &);

    /**
     * @brief subscribes to head changes
     * @param subscriber subscription handler
     * @return connection handle, which can be used to cancel subscription
     */
    virtual connection_t subscribeHeadChanges(
        const std::function<HeadChangeSignature> &subscriber) = 0;

    virtual outcome::result<void> updateHeaviestTipset(
        const Tipset &tipset) = 0;

    inline auto genesisCid() const {
      OUTCOME_EXCEPT(cid, primitives::cid::getCidOfCbor(getGenesis()));
      return cid;
    }
  };

}  // namespace fc::storage::blockchain

#endif  // CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_HPP
