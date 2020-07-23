/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_IMPL_HPP
#define CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_IMPL_HPP

#include "storage/chain/chain_store.hpp"

#include <map>

#include "blockchain/block_validator/block_validator.hpp"
#include "blockchain/weight_calculator.hpp"
#include "common/outcome.hpp"
#include "storage/chain/chain_data_store.hpp"
//#include "storage/ipfs/impl/ipfs_block_service.hpp"
#include "storage/indexdb/indexdb.hpp"
#include "sync/peer_manager.hpp"
#include "sync/tipset_loader.hpp"

namespace fc::storage::blockchain {

  using ::fc::blockchain::block_validator::BlockValidator;
  using ::fc::blockchain::weight::WeightCalculator;
  using ipfs::IpfsDatastore;
  using libp2p::peer::PeerId;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetHash;
  using primitives::tipset::TipsetKey;

  class ChainStoreImpl : public ChainStore,
                         public std::enable_shared_from_this<ChainStoreImpl> {
   public:
    ChainStoreImpl(ChainStoreImpl &&other) = default;

    ChainStoreImpl(std::shared_ptr<IpfsDatastore> data_store,
                   std::shared_ptr<BlockValidator> block_validator,
                   std::shared_ptr<WeightCalculator> weight_calculator,
                   std::shared_ptr<storage::indexdb::IndexDb> index_db,
                   std::shared_ptr<sync::TipsetLoader> tipset_loader,
                   std::shared_ptr<sync::PeerManager> peer_manager);

    outcome::result<void> init(BlockHeader genesis_header) override;

    //    outcome::result<void> writeHead(const Tipset &tipset);
    //
    outcome::result<Tipset> loadTipset(const TipsetKey &key) override;

    outcome::result<void> addBlock(const BlockHeader &block) override;

    outcome::result<Tipset> heaviestTipset() const override;
    //
    //    outcome::result<bool> containsTipset(const TipsetKey &key) const
    //    override;
    //
    //    outcome::result<BlockHeader> getGenesis() const override;

    outcome::result<TipsetKey> genesisTipsetKey() const override;
    outcome::result<CID> genesisCID() const override;

    //    outcome::result<void> writeGenesis(
    //        const BlockHeader &block_header) override;
    //
    //    primitives::BigInt getHeaviestWeight() const override {
    //      return heaviest_weight_;
    //    }
    //
    //    outcome::result<bool> contains(const CID &key) const override {
    //      return data_store_->contains(key);
    //    }
    //
    //    outcome::result<void> set(const CID &key, Value value) override {
    //      return data_store_->set(key, value);
    //    }
    //
    //    outcome::result<Value> get(const CID &key) const override {
    //      return data_store_->get(key);
    //    }
    //
    //    outcome::result<void> remove(const CID &key) override {
    //      return data_store_->remove(key);
    //    }
    //
    std::shared_ptr<ChainRandomnessProvider> createRandomnessProvider()
        override;

    connection_t subscribeHeadChanges(
        const std::function<HeadChangeSignature> &subscriber) override;

    //        {
    //          // TODO schedule
    //          if (heaviest_tipset_.has_value()) {
    //            subscriber(HeadChange{.type = HeadChangeType::CURRENT,
    //                                  .value = *heaviest_tipset_});
    //          }
    //          return head_change_signal_.connect(subscriber);
    //        }

   private:
    outcome::result<void> chooseHead();

    outcome::result<Tipset> loadTipsetLocally(const TipsetHash &hash,
                                              const std::vector<CID> &cids);

    void onTipsetAsyncLoaded(TipsetHash hash,
                             outcome::result<Tipset> tipset_res);

    void onSyncFinished(outcome::result<void> result);

    void synchronizeTipset(TipsetKey key);

    void choosePeer();

    outcome::result<void> applyHead(TipsetHash hash, std::vector<CID> cids);

    //    outcome::result<void> takeHeaviestTipset(const Tipset &tipset);
    //
    //    outcome::result<Tipset> expandTipset(const BlockHeader &block_header);
    //
    //    outcome::result<void> updateHeaviestTipset(const Tipset &tipset);
    //
    //    outcome::result<ChainPath> findChainPath(const Tipset &current,
    //                                             const Tipset &target);
    //
    //    outcome::result<void> notifyHeadChange(const Tipset &current,
    //                                           const Tipset &target);

    ///< main data storage
    std::shared_ptr<IpfsDatastore> data_store_;
    ///< wrapper around main data storage to store tipset keys
    // std::shared_ptr<ChainDataStore> chain_data_store_;
    std::shared_ptr<BlockValidator> block_validator_;
    std::shared_ptr<WeightCalculator> weight_calculator_;
    std::shared_ptr<storage::indexdb::IndexDb> index_db_;
    std::shared_ptr<sync::TipsetLoader> tipset_loader_;
    std::shared_ptr<sync::PeerManager> peer_manager_;

    storage::indexdb::Branches heads_;
    storage::indexdb::BranchInfo head_branch_;
    boost::optional<Tipset> heaviest_tipset_;  ///< current heaviest tipset
    primitives::BigInt heaviest_weight_{0};    ///< current heaviest weight
    boost::optional<Tipset> genesis_;          ///< genesis block
    boost::optional<CID> genesis_cid_;
    boost::optional<PeerId> current_peer_;
    boost::optional<Tipset> tipset_in_production_;
    boost::optional<TipsetHash> tipset_in_sync_;
    storage::indexdb::BranchId syncing_branch_ = storage::indexdb::kNoBranch;

    // std::unordered_map<uint64_t, std::vector<CID>> tipsets_;
    // mutable std::unordered_map<TipsetKey, Tipset> tipsets_cache_;

    /// when head tipset changes, need to notify all subscribers
    boost::signals2::signal<HeadChangeSignature> head_change_signal_;

    // common::Logger logger_;
  };
}  // namespace fc::storage::blockchain

#endif  // CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_IMPL_HPP
