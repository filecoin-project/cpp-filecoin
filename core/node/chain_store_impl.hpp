/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_CHAIN_STORE_IMPL_HPP
#define CPP_FILECOIN_SYNC_CHAIN_STORE_IMPL_HPP

#include "common.hpp"

#include "blockchain/block_validator/block_validator.hpp"
#include "blockchain/weight_calculator.hpp"
#include "head_constructor.hpp"
#include "storage/chain/chain_store.hpp"
#include "vm/interpreter/impl/interpreter_impl.hpp"

namespace fc::sync {
  using blockchain::block_validator::BlockValidator;
  using blockchain::weight::WeightCalculator;
  using vm::interpreter::CachedInterpreter;

  class ChainStoreImpl : public fc::storage::blockchain::ChainStore,
                         public std::enable_shared_from_this<ChainStoreImpl> {
   public:
    ChainStoreImpl(std::shared_ptr<storage::ipfs::IpfsDatastore> ipld,
                   std::shared_ptr<CachedInterpreter> interpreter,
                   std::shared_ptr<WeightCalculator> weight_calculator,
                   std::shared_ptr<BlockValidator> block_validator);

    outcome::result<void> start(std::shared_ptr<events::Events> events);

    outcome::result<void> addBlock(const BlockHeader &block) override;

    TipsetCPtr heaviestTipset() const override;

    connection_t subscribeHeadChanges(
        const std::function<HeadChangeSignature> &subscriber) override;

    primitives::BigInt getHeaviestWeight() const override;

   private:
    void newHeadChosen(TipsetCPtr tipset, BigInt weight);

    HeadConstructor head_constructor_;
    std::shared_ptr<storage::ipfs::IpfsDatastore> ipld_;
    std::shared_ptr<CachedInterpreter> interpreter_;
    std::shared_ptr<WeightCalculator> weight_calculator_;
    std::shared_ptr<BlockValidator> block_validator_;

    std::shared_ptr<events::Events> events_;

    TipsetCPtr head_;
    BigInt heaviest_weight_;

    boost::signals2::signal<HeadChangeSignature> head_change_signal_;
  };
}  // namespace fc::sync

#endif  // CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_IMPL_HPP
