/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_CHAIN_STORE_IMPL_HPP
#define CPP_FILECOIN_SYNC_CHAIN_STORE_IMPL_HPP

#include "fwd.hpp"

#include "storage/chain/chain_store.hpp"
#include "blockchain/weight_calculator.hpp"
#include "head_constructor.hpp"

namespace fc::sync {
  using blockchain::weight::WeightCalculator;

  class ChainDb;

  class ChainStoreImpl : public fc::storage::blockchain::ChainStore {
   public:
    ChainStoreImpl(std::shared_ptr<ChainDb> chain_db,
                   std::shared_ptr<storage::ipfs::IpfsDatastore> ipld,
                   std::shared_ptr<WeightCalculator> weight_calculator);

    void start(std::shared_ptr<events::Events> events, std::string network_name);

    outcome::result<void> addBlock(const BlockHeader &block) override;

    outcome::result<TipsetCPtr> loadTipset(const TipsetHash &hash) override;

    outcome::result<TipsetCPtr> loadTipset(const TipsetKey &key) override;

    outcome::result<TipsetCPtr> loadTipsetByHeight(uint64_t height) override;

    TipsetCPtr heaviestTipset() const override;

    connection_t subscribeHeadChanges(
        const std::function<HeadChangeSignature> &subscriber) override;

    const std::string& getNetworkName() const override;

    const CID& genesisCID() const override;

    primitives::BigInt getHeaviestWeight() const override;

   private:
    HeadConstructor head_constructor_;
    std::shared_ptr<ChainDb> chain_db_;
    std::shared_ptr<storage::ipfs::IpfsDatastore> ipld_;
    std::shared_ptr<WeightCalculator> weight_calculator_;

    std::shared_ptr<events::Events> events_;

    std::string network_name_;

    TipsetCPtr head_;
    BigInt heaviest_weight_;

    boost::signals2::signal<HeadChangeSignature> head_change_signal_;
  };
}  // namespace fc::storage::blockchain

#endif  // CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_IMPL_HPP
